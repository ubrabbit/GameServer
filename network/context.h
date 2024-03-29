#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <list>
#include <map>

#include "common/logger.h"
#include "common/noncopyable.h"
#include "common/utils/spinlock.h"
#include "common/utils/uuid.h"
#include "common/signal.h"

#include "defines.h"
#include "packet.h"
#include "unpack.h"
#include "queue.h"

namespace network
{

// 结构体必须是POD类型
typedef struct ST_ClientContextBuffer
{
public:
	uint64_t m_ulClientSeq;
	int32_t  m_iClientFd;
	char 	 m_chClientIP[NETWORK_SERVER_IP_ADDRESS_LEN];

	int32_t  m_iBufferSize;
	BYTE     m_chBuffer[NETWORK_PACKET_BUFFER_READ_SIZE * 10];

	void construct()
	{
		memset(this, 0, sizeof(*this));
	}

	ST_ClientContextBuffer()
	{
		construct();
	}

	~ST_ClientContextBuffer()
	{
		construct();
	}

	int32_t GetClientFd()
	{
		return m_iClientFd;
	}

	uint64_t GetClientSeq()
	{
		return m_ulClientSeq;
	}

	BYTE* GetBufferPtr()
	{
		return m_chBuffer;
	}

	int32_t GetBufferSize()
	{
		return m_iBufferSize;
	}

	void MoveBufferPtr(BYTE* pchBufferPtr, int32_t iLeftSize)
	{
		if(iLeftSize <= 0)
		{
			memset(m_chBuffer, 0, sizeof(m_chBuffer));
			m_iBufferSize = 0;
			return;
		}

		//把剩余的缓存数据往前移
		memmove(m_chBuffer, pchBufferPtr, iLeftSize);
		m_iBufferSize = iLeftSize;
	}

}ST_ClientContextBuffer;


class ClientContext : public noncopyable
{
public:
	virtual ~ClientContext() {};

	virtual ST_ClientContextBuffer* GetContextBuffer() = 0;
	virtual size_t PacketBufferSend(NetPacketBuffer& rstPacket) = 0;

	virtual const char* GetClientIP() = 0;
	virtual const int32_t GetClientFd() = 0;
	virtual const uint64_t GetClientSeq() = 0;

};


class ClientContextPool
{
public:
	std::map<uint64_t, ClientContext*> m_stClientSeqPool;
	std::map<int32_t, ClientContext*>  m_stClientFdPool;

	ClientContext* GetClient(uint64_t ulClientSeq)
	{
		auto it = m_stClientSeqPool.find(ulClientSeq);
		if(it != m_stClientSeqPool.end())
		{
			return it->second;
		}
		return NULL;
	}

	ClientContext* GetClientByFd(int32_t iClientFd)
	{
		auto it = m_stClientFdPool.find(iClientFd);
		if(it != m_stClientFdPool.end())
		{
			return it->second;
		}
		return NULL;
	}

	ClientContext* RemoveClientByFd(int32_t iClientFd)
	{
		auto it = m_stClientFdPool.find(iClientFd);
		if(it != m_stClientFdPool.end())
		{
			m_stClientFdPool.erase(it);
			return it->second;
		}
		return NULL;
	}

	void AddClient(uint64_t ulClientSeq, ClientContext* pstClient)
	{
		m_stClientSeqPool.insert(std::pair<uint64_t, ClientContext*>(ulClientSeq, pstClient));
		m_stClientFdPool.insert(std::pair<int32_t, ClientContext*>(pstClient->GetClientFd(), pstClient));
		return;
	}

	ClientContext* RemoveClient(uint64_t ulClientSeq)
	{
		auto it = m_stClientSeqPool.find(ulClientSeq);
		if(it != m_stClientSeqPool.end())
		{
			ClientContext* pstClient = it->second;
			RemoveClientByFd(pstClient->GetClientFd());
			m_stClientSeqPool.erase(it);
			return pstClient;
		}
		return NULL;
	}

};


class NetworkContext : public noncopyable
{
public:
	struct spinlock		m_stReadLock;
	struct spinlock		m_stWriteLock;

    // 网路线程发包预备队列
	NetPacketQueue		m_stNetPacketRecvPrepareQueue;
    // 网络线程收包队列
	NetPacketQueue 		m_stNetPacketRecvQueue;
    // 逻辑线程处理缓存
	NetPacketQueue 		m_stNetPacketLogicQueue;

    // 网路线程发包预备队列
	NetPacketQueue		m_stNetPacketSendPrepareQueue;
    // 网路线程发包队列
	NetPacketQueue		m_stNetPacketSendQueue;
    // 网路线程发包缓存
	NetPacketQueue		m_stNetPacketSendCache;

	ST_GameSingnal      m_stSignalExec;

	NetworkContext()
	{
		spinlock_init(&m_stReadLock);
		spinlock_init(&m_stWriteLock);
	}

	virtual ~NetworkContext()
	{
		spinlock_destroy(&m_stReadLock);
		spinlock_destroy(&m_stWriteLock);
	}

    int32_t PacketRecvPrepare()
	{
		spinlock_lock(&m_stReadLock);
		int32_t iPacketRecvNum = m_stNetPacketRecvQueue.CopyFrom(m_stNetPacketRecvPrepareQueue);
		spinlock_unlock(&m_stReadLock);
		return iPacketRecvNum;
	}

    int32_t CopyPacketsFromNetToLogic()
	{
		spinlock_lock(&m_stReadLock);
		m_stNetPacketLogicQueue.Init();
		int32_t iPacketRecvNum = m_stNetPacketLogicQueue.CopyFrom(m_stNetPacketRecvQueue);
		spinlock_unlock(&m_stReadLock);
		return iPacketRecvNum;
	}

	template<class T>
    bool PacketProduceProtobufPacket(uint64_t ulClientSeq, int32_t iClientFd, int32_t iProtoNo, T& rstProto)
	{
		int32_t iBufferSize = (int32_t)rstProto.ByteSizeLong();
		BYTE chRspBuffer[iBufferSize];
		memset(chRspBuffer, 0, sizeof(chRspBuffer));
		PackProtobufStruct(iProtoNo, rstProto, iBufferSize, iBufferSize, chRspBuffer);
		m_stNetPacketSendPrepareQueue.AddPacket(ulClientSeq, iClientFd, iProtoNo, iBufferSize, (BYTE*)chRspBuffer);

		return true;
	}

    int32_t PacketSendPrepare()
	{
		spinlock_lock(&m_stWriteLock);
		int32_t iPacketRecvNum = m_stNetPacketSendQueue.CopyFrom(m_stNetPacketSendPrepareQueue);
		spinlock_unlock(&m_stWriteLock);
		return iPacketRecvNum;
	}

    int32_t PacketSendPrepareCache()
	{
		spinlock_lock(&m_stWriteLock);
		m_stNetPacketSendCache.CopyFrom(m_stNetPacketSendQueue);
		int32_t iPacketRecvNum = m_stNetPacketSendCache.Size();
		spinlock_unlock(&m_stWriteLock);
		return iPacketRecvNum;
	}

    int32_t UnpackPacketToRecvQueue(ClientContext* pstClientCtx)
	{
		assert(pstClientCtx);

		size_t dwStartTime = GetMilliSecond();
		int32_t iTotalPackets = 0;

		ST_ClientContextBuffer* pstContextBuffer = pstClientCtx->GetContextBuffer();
		if(NULL == pstContextBuffer)
		{
			LOGFATAL("client unpack packet error by context buffer is NULL!");
			return -1;
		}
		ST_ClientContextBuffer& rstContextBuffer = *pstContextBuffer;

		int32_t iClientFd = rstContextBuffer.GetClientFd();
		uint64_t ulClientSeq = rstContextBuffer.GetClientSeq();
		int32_t iLeftSize = rstContextBuffer.GetBufferSize();
		BYTE* pchBufferPtr = rstContextBuffer.GetBufferPtr();
		while(iLeftSize >= NETWORK_PACKET_HEADER_SIZE)
		{
			BYTE* ptr = pchBufferPtr;
			int32_t iPacketSize = (int32_t)UnpackInt(ptr, (int16_t)sizeof(int16_t));
			// 还没收到足够长度的包
			if(iLeftSize < iPacketSize || iPacketSize < NETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			int32_t iProtoNo = (int32_t)UnpackInt(ptr, (int16_t)sizeof(int16_t));
			int32_t iBodySize = iPacketSize - NETWORK_PACKET_HEADER_SIZE;
			if(0 >= iBodySize)
			{
				LOGERROR("client<{}><{}> unpack packet error by body size {} not in valid range.", ulClientSeq, iClientFd, iBodySize);
				return -2;
			}
			if(NETWORK_PACKET_BUFFER_MAX_SIZE < iBodySize)
			{
				LOGERROR("client<{}><{}> unpack packet error by body size {} exceed max.", ulClientSeq, iClientFd, iBodySize);
				return -3;
			}
			m_stNetPacketRecvPrepareQueue.AddPacket(ulClientSeq, iClientFd, iProtoNo, iBodySize, ptr);

			pchBufferPtr += iPacketSize;
			iLeftSize -= iPacketSize;
			iTotalPackets++;
		}
		rstContextBuffer.MoveBufferPtr(pchBufferPtr, iLeftSize);

		PacketRecvPrepare();

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets > 0 && dwCostTime > 10)
		{
			LOGINFO("UnpackPacketFromRecvBuffer Finished: TotalPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		if(iTotalPackets > 0)
		{
			m_stSignalExec.Emit();
		}

		return 0;
	}

	virtual int32_t ExecuteCmdRecvAllPacket(ClientContext* pstClientCtx) = 0;
	virtual size_t ExecuteCmdSendAllPacket(struct ST_NETWORK_CMD_REQUEST& rstNetCmd) = 0;

};

class ServerContext : public NetworkContext
{
public:
	ClientContextPool 	m_stClientPool;

	int32_t ExecuteCmdRecvAllPacket(ClientContext* pstClientCtx)
	{
		return UnpackPacketToRecvQueue(pstClientCtx);
	}

	size_t ExecuteCmdSendAllPacket(struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
	{
		size_t dwStartTime = GetMilliSecond();

		int32_t iTotalPackets = PacketSendPrepareCache();
		do {
			for(auto it=m_stNetPacketSendCache.m_vecPacketPool.begin(); it!=m_stNetPacketSendCache.m_vecPacketPool.end(); it++)
			{
				NetPacketBuffer& rstPacket = *it;
				ClientContext* pstClientCtx = m_stClientPool.GetClient(rstPacket.GetClientSeq());
				if(NULL == pstClientCtx)
				{
					continue;
				}
				pstClientCtx->PacketBufferSend(rstPacket);
			}
			m_stNetPacketSendCache.ReleaseAllPackets();
			iTotalPackets = PacketSendPrepareCache();
		} while (iTotalPackets > 0);

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets > 0 && dwCostTime > 10)
		{
			LOGINFO("ExecuteCmdSendAllPacket Finished: SendPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		return iTotalPackets;
	}

	const std::string GetClientIP(uint64_t ulClientSeq)
	{
		ClientContext* pstClientCtx = m_stClientPool.GetClient(ulClientSeq);
		if(NULL == pstClientCtx)
		{
			return std::string("Unknown");
		}
		return std::string(pstClientCtx->GetClientIP());
	}
};

class ConnectorContext : public NetworkContext
{
public:
	uint64_t 		m_ulClientSeq;
	ClientContext*  m_pstClientCtx;

	ConnectorContext(uint64_t ulClientSeq, ClientContext* pstClientCtx):
		m_ulClientSeq(ulClientSeq),
		m_pstClientCtx(pstClientCtx)
	{}

	~ConnectorContext()
	{
		m_pstClientCtx = NULL;
	}

	int32_t ExecuteCmdRecvAllPacket(ClientContext* pstClientCtx)
	{
		return UnpackPacketToRecvQueue(pstClientCtx);
	}

	size_t ExecuteCmdSendAllPacket(struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
	{
		size_t dwStartTime = GetMilliSecond();

		if (NULL == m_pstClientCtx)
		{
			LOGFATAL("ExecuteCmdSendAllPacket but client ctx is NUll!");
			return -1;
		}

		int32_t iTotalPackets = PacketSendPrepareCache();
		do {
			for(auto it=m_stNetPacketSendCache.m_vecPacketPool.begin(); it!=m_stNetPacketSendCache.m_vecPacketPool.end(); it++)
			{
				NetPacketBuffer& rstPacket = *it;
				m_pstClientCtx->PacketBufferSend(rstPacket);
			}
			m_stNetPacketSendCache.ReleaseAllPackets();
			iTotalPackets = PacketSendPrepareCache();
		} while (iTotalPackets > 0);

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets >= 0 && dwCostTime >= 10)
		{
			LOGINFO("ExecuteCmdSendAllPacket Finished: SendPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		return iTotalPackets;
	}

	const std::string GetClientIP(uint64_t ulClientSeq)
	{
		if(NULL == m_pstClientCtx)
		{
			return std::string("Unknown");
		}
		return std::string(m_pstClientCtx->GetClientIP());
	}

	const uint64_t GetClientSeq()
	{
		return m_ulClientSeq;
	}

};

class NertworkServer
{
public:
    virtual ~NertworkServer(){};

    virtual void SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd) = 0;
    virtual void RecvNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd) = 0;

};

} // namespace network

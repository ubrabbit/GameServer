#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <list>
#include <map>

#include "common/logger.h"
#include "common/noncopyable.h"
#include "common/utils/spinlock.h"
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

};


class ClientContextPool
{
public:
	std::map<int32_t, ClientContext*> m_stPool;

	ClientContext* GetClient(int32_t iSockFd)
	{
		auto it = m_stPool.find(iSockFd);
		if(it != m_stPool.end())
		{
			return it->second;
		}
		return NULL;
	}

	void AddClient(int32_t iSockFd, ClientContext* pstClient)
	{
		m_stPool.insert(std::pair<int, ClientContext*>(iSockFd, pstClient));
		return;
	}

	ClientContext* RemoveClient(int32_t iSockFd)
	{
		auto it = m_stPool.find(iSockFd);
		if(it != m_stPool.end())
		{
			ClientContext* pstClient = it->second;
			m_stPool.erase(it);
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

    // 网络线程收包缓存
	NetPacketQueue 		m_stNetPacketRecvQueue;
    // 逻辑线程处理缓存
	NetPacketQueue 		m_stNetPacketLogicQueue;

    // 网路线程发包队列
	NetPacketQueue		m_stNetPacketSendQueue;

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

    void AddPacketToRecvQueue(int32_t iSockFd, int16_t wProtoNo, int32_t iBufferSize, BYTE* pchBuffer)
	{
		spinlock_lock(&m_stReadLock);
		m_stNetPacketRecvQueue.AddPacket(iSockFd, wProtoNo, iBufferSize, (BYTE*)pchBuffer);
		spinlock_unlock(&m_stReadLock);
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
    bool PacketProduceProtobufPacket(int32_t iSockFd, uint16_t wProtoNo, T& rstProto)
	{
		int32_t iBufferSize = (int32_t)rstProto.ByteSizeLong();
		BYTE chRspBuffer[iBufferSize];
		memset(chRspBuffer, 0, sizeof(chRspBuffer));
		PackProtobufStruct(wProtoNo, rstProto, iBufferSize, chRspBuffer);

		spinlock_lock(&m_stWriteLock);
		m_stNetPacketSendQueue.AddPacket(iSockFd, wProtoNo, iBufferSize, (BYTE*)chRspBuffer);
		spinlock_unlock(&m_stWriteLock);

		return true;
	}

    int32_t PacketSendPrepare()
	{
		spinlock_lock(&m_stWriteLock);
		int32_t iPacketRecvNum = m_stNetPacketSendQueue.Size();
		spinlock_unlock(&m_stWriteLock);
		return iPacketRecvNum;
	}

    int32_t CopyPacketsFromLogicToNet(std::vector<NetPacketBuffer>& vecNetPacketCacheQueue)
	{
		spinlock_lock(&m_stWriteLock);
		vecNetPacketCacheQueue.swap(m_stNetPacketSendQueue.m_vecPacketPool);
		m_stNetPacketSendQueue.Init();
		int32_t iPacketRecvNum = vecNetPacketCacheQueue.size();
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
		int32_t iLeftSize = rstContextBuffer.GetBufferSize();
		BYTE* pchBufferPtr = rstContextBuffer.GetBufferPtr();
		while(iLeftSize >= NETWORK_PACKET_HEADER_SIZE)
		{
			int32_t iPacketSize = (int32_t)UnpackInt(pchBufferPtr, (int16_t)sizeof(int16_t));
			// 还没收到足够长度的包
			if(iLeftSize < iPacketSize || iPacketSize < NETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			int32_t iProtoNo = (int32_t)UnpackInt(pchBufferPtr + sizeof(int16_t), (int16_t)sizeof(int16_t));
			int16_t wBodySize = (int16_t)(iPacketSize - NETWORK_PACKET_HEADER_SIZE);
			if(wBodySize <= 0)
			{
				LOGERROR("client<{}> unpack packet error by body size {} not in valid range.", iClientFd, wBodySize);
				return -2;
			}

			BYTE* pstBufferData = pchBufferPtr + NETWORK_PACKET_HEADER_SIZE;
			AddPacketToRecvQueue(iClientFd, (int16_t)iProtoNo, (int16_t)wBodySize, pstBufferData);

			pchBufferPtr += iPacketSize;
			iLeftSize -= iPacketSize;
			iTotalPackets++;
		}
		rstContextBuffer.MoveBufferPtr(pchBufferPtr, iLeftSize);

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

		std::vector<NetPacketBuffer> vecNetPacketSendBuffer;
		CopyPacketsFromLogicToNet(vecNetPacketSendBuffer);

		int32_t iTotalPackets = vecNetPacketSendBuffer.size();
		for(auto it=vecNetPacketSendBuffer.begin(); it!=vecNetPacketSendBuffer.end(); it++)
		{
			NetPacketBuffer& rstPacket = *it;
			int32_t iClientFd = rstPacket.GetClientFd();
			ClientContext* pstClientCtx = m_stClientPool.GetClient(iClientFd);
			if(NULL == pstClientCtx)
			{
				continue;
			}
			pstClientCtx->PacketBufferSend(rstPacket);
		}
		vecNetPacketSendBuffer.clear();

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets > 0 && dwCostTime > 10)
		{
			LOGINFO("ExecuteCmdSendAllPacket Finished: SendPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		return iTotalPackets;
	}
};

class ConnectorContext : public NetworkContext
{
public:

	ClientContext* m_pstClientCtx;

	ConnectorContext(ClientContext* pstClientCtx):
		m_pstClientCtx(pstClientCtx)
	{
	}

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

		std::vector<NetPacketBuffer> vecNetPacketSendBuffer;
		CopyPacketsFromLogicToNet(vecNetPacketSendBuffer);
		int32_t iTotalPackets = vecNetPacketSendBuffer.size();
		for(auto it=vecNetPacketSendBuffer.begin(); it!=vecNetPacketSendBuffer.end(); it++)
		{
			NetPacketBuffer& rstPacket = *it;
			m_pstClientCtx->PacketBufferSend(rstPacket);
		}
		vecNetPacketSendBuffer.clear();

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets >= 0 && dwCostTime >= 0)
		{
			LOGINFO("ExecuteCmdSendAllPacket Finished: SendPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		return iTotalPackets;
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

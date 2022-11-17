#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <list>
#include <map>

#include "common/logger.h"
#include "common/noncopyable.h"
#include "common/utils/spinlock.h"

#include "defines.h"
#include "packet.h"
#include "unpack.h"

namespace network {

// ST_PacketBufferPool 后续拓展必须维持POD结构！
typedef struct ST_PacketBufferPool
{
public:

    int32_t m_iPacketRecvNum;
    NetPacketBuffer m_stPacketBufferPool[NETWORK_PACKET_BUFFER_POOL_SIZE];

    void construct()
    {
        memset(this, 0, sizeof(*this));
    }

	int32_t size()
	{
		return NETWORK_PACKET_BUFFER_POOL_SIZE;
	}

    ST_PacketBufferPool()
    {
        construct();
    }

} ST_PacketBufferPool;


class ClientPool
{
public:
	std::map<int32_t, void*> m_mstClientPool;

	void* GetClient(int32_t iSockFd)
	{
		auto it = m_mstClientPool.find(iSockFd);
		if(it != m_mstClientPool.end())
		{
			return it->second;
		}
		return NULL;
	}

	void AddClient(int32_t iSockFd, void* pstClient)
	{
		m_mstClientPool.insert(std::pair<int, void*>(iSockFd, pstClient));
		return;
	}

	void* RemoveClient(int32_t iSockFd)
	{
		auto it = m_mstClientPool.find(iSockFd);
		if(it != m_mstClientPool.end())
		{
			void* pstClient = it->second;
			m_mstClientPool.erase(it);
			return pstClient;
		}
		return NULL;
	}

};


class ServerNetBufferCtx : public noncopyable
{
public:
	struct spinlock		m_stReadLock;
	struct spinlock		m_stWriteLock;
	struct spinlock		m_stResourceLock;

    // 网络线程收包缓存
    ST_PacketBufferPool 		  m_stNetPacketBufferPoolRecv;

    // 网路线程发包缓存
	std::vector<NetPacketBuffer>  m_vecNetPacketSendBuffer;

	ClientPool 					m_stClientPool;

	ServerNetBufferCtx()
	{
		spinlock_init(&m_stReadLock);
		spinlock_init(&m_stWriteLock);
		spinlock_init(&m_stResourceLock);
	}

	~ServerNetBufferCtx()
	{
		spinlock_destroy(&m_stReadLock);
		spinlock_destroy(&m_stWriteLock);
		spinlock_destroy(&m_stResourceLock);
	}

    NetPacketBuffer* RequireNetPacketSlot(int32_t iSockFd, int16_t wProtoNo, int16_t wBufferSize, BYTE* pchBuffer)
	{
		if (m_stNetPacketBufferPoolRecv.m_iPacketRecvNum >= m_stNetPacketBufferPoolRecv.size())
		{
			return NULL;
		}

		spinlock_lock(&m_stReadLock);
		NetPacketBuffer& rstPacket = m_stNetPacketBufferPoolRecv.m_stPacketBufferPool[m_stNetPacketBufferPoolRecv.m_iPacketRecvNum];
		rstPacket.Init(iSockFd, wProtoNo, wBufferSize, pchBuffer);
		m_stNetPacketBufferPoolRecv.m_iPacketRecvNum++;
		spinlock_unlock(&m_stReadLock);
		return &rstPacket;
	}

    int32_t PacketCopyFromNetToLogic(ST_PacketBufferPool& rstLogicBufferPool)
	{
		rstLogicBufferPool.construct();

		spinlock_lock(&m_stReadLock);
		rstLogicBufferPool.m_iPacketRecvNum = m_stNetPacketBufferPoolRecv.m_iPacketRecvNum;
		memcpy(rstLogicBufferPool.m_stPacketBufferPool, m_stNetPacketBufferPoolRecv.m_stPacketBufferPool, sizeof(NetPacketBuffer) * m_stNetPacketBufferPoolRecv.m_iPacketRecvNum);
		m_stNetPacketBufferPoolRecv.construct();
		spinlock_unlock(&m_stReadLock);

		return rstLogicBufferPool.m_iPacketRecvNum;
	}

	template<class T>
    bool PacketSendProduceProtobufPacket(int32_t iSockFd, uint16_t wProtoNo, T& rstProto)
	{
		int32_t iBufferSize = (int32_t)rstProto.ByteSizeLong();
		BYTE chRspBuffer[iBufferSize];
		memset(chRspBuffer, 0, sizeof(chRspBuffer));
		PackProtobufStruct(wProtoNo, rstProto, iBufferSize, chRspBuffer);

		spinlock_lock(&m_stWriteLock);
		m_vecNetPacketSendBuffer.emplace_back(iSockFd, wProtoNo, iBufferSize, (BYTE*)chRspBuffer);
		spinlock_unlock(&m_stWriteLock);

		return true;
	}

    int32_t PacketSendPrepare(std::vector<NetPacketBuffer>& rvecNetPacketSendBuffer)
	{
		spinlock_lock(&m_stWriteLock);
		rvecNetPacketSendBuffer.swap(m_vecNetPacketSendBuffer);
		m_vecNetPacketSendBuffer.clear();
		spinlock_unlock(&m_stWriteLock);
		return 0;
	}

	bool HasPacketToSend()
	{
		spinlock_lock(&m_stWriteLock);
		int iListSize = m_vecNetPacketSendBuffer.size();
		spinlock_unlock(&m_stWriteLock);

		return iListSize > 0;
	}

};

} // namespace network

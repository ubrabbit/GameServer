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
#include "queue.h"

namespace network {


class ClientContextPool
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


class ServerContext : public noncopyable
{
public:
	struct spinlock		m_stReadLock;
	struct spinlock		m_stWriteLock;

    // 网络线程收包缓存
	NetPacketQueue 				  m_stNetPacketRecvQueue;
    // 逻辑线程处理缓存
	NetPacketQueue 				  m_stNetPacketLogicQueue;

    // 网路线程发包队列
	NetPacketQueue				  m_stNetPacketWaitQueue;
	NetPacketQueue				  m_stNetPacketSendQueue;

	ClientContextPool 			  m_stClientPool;

	ServerContext()
	{
		spinlock_init(&m_stReadLock);
		spinlock_init(&m_stWriteLock);
	}

	~ServerContext()
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

    int32_t PacketCopyFromNetToLogic()
	{
		spinlock_lock(&m_stReadLock);
		m_stNetPacketLogicQueue.Init();
		int32_t iPacketRecvNum = m_stNetPacketLogicQueue.CopyFrom(m_stNetPacketRecvQueue);
		spinlock_unlock(&m_stReadLock);
		return iPacketRecvNum;
	}

	template<class T>
    bool PacketSendProduceProtobufPacket(int32_t iSockFd, uint16_t wProtoNo, T& rstProto)
	{
		int32_t iBufferSize = (int32_t)rstProto.ByteSizeLong();
		BYTE chRspBuffer[iBufferSize];
		memset(chRspBuffer, 0, sizeof(chRspBuffer));
		PackProtobufStruct(wProtoNo, rstProto, iBufferSize, chRspBuffer);

		m_stNetPacketWaitQueue.AddPacket(iSockFd, wProtoNo, iBufferSize, (BYTE*)chRspBuffer);

		return true;
	}

    int32_t PacketSendPrepare()
	{
		spinlock_lock(&m_stWriteLock);
		m_stNetPacketSendQueue.CopyFrom(m_stNetPacketWaitQueue);
		spinlock_unlock(&m_stWriteLock);
		return 0;
	}

    int32_t CopyPacketsFromLogicToNet(std::vector<NetPacketBuffer>& vecNetPacketCacheQueue)
	{
		spinlock_lock(&m_stWriteLock);
		vecNetPacketCacheQueue.swap(m_stNetPacketSendQueue.m_vecPacketPool);
		m_stNetPacketSendQueue.Init();
		spinlock_unlock(&m_stWriteLock);
		return 0;
	}

	bool HasPacketsWaiting()
	{
		return m_stNetPacketWaitQueue.Size() > 0;
	}

};

} // namespace network

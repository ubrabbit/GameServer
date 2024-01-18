#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <list>

#include "common/logger.h"
#include "common/noncopyable.h"
#include "common/utils/spinlock.h"

#include "defines.h"
#include "packet.h"
#include "unpack.h"

namespace network {

// 使用它需要在外面加锁
class NetPacketQueue
{
public:
    int32_t                       m_iPacketNum;
    std::vector<NetPacketBuffer>  m_vecPacketPool;

    NetPacketQueue()
    {
        Init();
    }

    ~NetPacketQueue()
    {
        m_vecPacketPool.clear();
    }

	int32_t Size()
	{
        return m_iPacketNum;
	}

    void Init()
    {
        m_vecPacketPool.clear();
        m_vecPacketPool.reserve(NETWORK_PACKET_BUFFER_POOL_SIZE);
        m_iPacketNum = 0;
    }

    int32_t CopyFrom(NetPacketQueue& rstPacketQueue)
    {
        if (m_iPacketNum > 0)
        {
            m_vecPacketPool.insert(m_vecPacketPool.end(), rstPacketQueue.m_vecPacketPool.begin(), rstPacketQueue.m_vecPacketPool.end());
            m_iPacketNum = m_vecPacketPool.size();
            rstPacketQueue.Init();
            return m_iPacketNum;
        }
        m_vecPacketPool.swap(rstPacketQueue.m_vecPacketPool);
        m_iPacketNum = m_vecPacketPool.size();
        rstPacketQueue.Init();
        return m_iPacketNum;
    }

    int32_t AddPacket(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize, BYTE* pchBuffer)
	{
        auto pstMemory = CPacketMemoryPool::Instance().GetFreePacketMemory();
        m_vecPacketPool.emplace_back(ulClientSeq, iSockFd, iProtoNo, iBufferSize, pstMemory, pchBuffer);
		m_iPacketNum++;
		return m_iPacketNum;
	}

};

} // namespace network

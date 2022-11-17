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
    int32_t                       m_iPacketRecvNum;
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
        return m_iPacketRecvNum;
	}

    void Init()
    {
        m_vecPacketPool.reserve(NETWORK_PACKET_BUFFER_POOL_SIZE);
        m_vecPacketPool.shrink_to_fit();
        m_iPacketRecvNum = 0;
    }

    int32_t CopyFrom(NetPacketQueue& rstPacketQueue)
    {
        if (m_iPacketRecvNum > 0)
        {
            for(int32_t i=0; i<rstPacketQueue.Size(); i++)
            {
                AddPacket(rstPacketQueue.m_vecPacketPool[i]);
            }
        }
        else
        {
            m_vecPacketPool.swap(rstPacketQueue.m_vecPacketPool);
            m_iPacketRecvNum = rstPacketQueue.m_iPacketRecvNum;
        }
        rstPacketQueue.Init();
        return m_iPacketRecvNum;
    }

    int32_t AddPacket(int32_t iSockFd, int16_t wProtoNo, int32_t iBufferSize, BYTE* pchBuffer)
	{
        if(m_iPacketRecvNum >= (int32_t)m_vecPacketPool.size())
        {
            m_vecPacketPool.emplace_back(iSockFd, wProtoNo, (int16_t)iBufferSize, pchBuffer);
        }
        else
        {
            m_vecPacketPool[m_iPacketRecvNum].Init(iSockFd, wProtoNo, (int16_t)iBufferSize, pchBuffer);
        }
		m_iPacketRecvNum++;
		return m_iPacketRecvNum;
	}

    int32_t AddPacket(NetPacketBuffer& rstPacket)
	{
        if(m_iPacketRecvNum >= (int32_t)m_vecPacketPool.size())
        {
            m_vecPacketPool.emplace_back(rstPacket);
        }
        else
        {
            m_vecPacketPool[m_iPacketRecvNum] = rstPacket;
        }
		m_iPacketRecvNum++;
		return m_iPacketRecvNum;
	}

};

} // namespace network

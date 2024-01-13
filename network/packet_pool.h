#pragma once

#include <deque>

#include "common/utils/time.h"
#include "defines.h"
#include "unpack.h"


namespace network {

#define MAX_PACKET_MEMORY_POOL_SIZE (1024)

typedef struct ST_PacketMemory
{
public:
    int32_t m_iIndex;
    BYTE    m_achBytesArray[NETWORK_PACKET_BUFFER_DEFAULT_SIZE];
    BYTE*   m_pchBytesExtraSpace;

    void construct()
    {
        memset(this, 0, sizeof(*this));
    }

    void release()
    {
        if(NULL != m_pchBytesExtraSpace)
        {
            delete[] m_pchBytesExtraSpace;
            m_pchBytesExtraSpace = NULL;
        }
        construct();
    }

    ST_PacketMemory()
    {
        construct();
    }

    ~ST_PacketMemory()
    {
        release();
    }

    BYTE* GetBuffer()
    {
        if(NULL != m_pchBytesExtraSpace)
        {
            return m_pchBytesExtraSpace;
        }
        return m_achBytesArray;
    }

    void Init(int32_t iBufferSize, BYTE* pchBuffer)
    {
		if((size_t)iBufferSize >= sizeof(m_achBytesArray))
		{
            assert(NETWORK_PACKET_BUFFER_MAX_SIZE >= iBufferSize);
			m_pchBytesExtraSpace = new BYTE[iBufferSize];
			memcpy(m_pchBytesExtraSpace, pchBuffer, iBufferSize);
		}
		else
		{
			memcpy(m_achBytesArray, pchBuffer, iBufferSize);
		}
    }

    void SetIndex(int32_t iIndex)
    {
        m_iIndex = iIndex;
    }

}ST_PacketMemory;

class CPacketMemoryPool : public Singleton<CPacketMemoryPool>
{
public:
    struct spinlock		m_stLock;

    ST_PacketMemory m_PacketMemoryPool[MAX_PACKET_MEMORY_POOL_SIZE];
    std::deque<size_t> m_PacketIndexFreeQueue;

    CPacketMemoryPool()
    {
        spinlock_init(&m_stLock);
        for(size_t i = 0; i < MAX_PACKET_MEMORY_POOL_SIZE; i++)
        {
            m_PacketIndexFreeQueue.push_back(i);
        }
    }

    ~CPacketMemoryPool()
    {
        spinlock_destroy(&m_stLock);
    }

    ST_PacketMemory* GetFreePacketMemory()
    {
        ST_PacketMemory* pstMemory = NULL;

        spinlock_lock(&m_stLock);
        bool bIsEmpty = m_PacketIndexFreeQueue.empty();
        if(bIsEmpty)
        {
            pstMemory = new ST_PacketMemory();
            pstMemory->SetIndex(-1);
        }
        else
        {
            auto iIndex = m_PacketIndexFreeQueue.front();
            m_PacketIndexFreeQueue.pop_front();
            ST_PacketMemory& rstMemory = m_PacketMemoryPool[iIndex];
            rstMemory.construct();
            rstMemory.SetIndex(iIndex);
            pstMemory = &rstMemory;
        }
        spinlock_unlock(&m_stLock);

        return pstMemory;

    }

    void ReleasePacketMemory(ST_PacketMemory* pstMemory)
    {
        assert(NULL != pstMemory);

        auto iIndex = pstMemory->m_iIndex;
        pstMemory->release();

        if(iIndex <= -1)
        {
            delete pstMemory;
            return;
        }

        spinlock_lock(&m_stLock);
        m_PacketIndexFreeQueue.push_back(iIndex);
        spinlock_unlock(&m_stLock);

    }

};


} // namespace network

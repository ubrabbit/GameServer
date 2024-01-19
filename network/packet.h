#pragma once

#include "common/utils/time.h"
#include "defines.h"
#include "unpack.h"
#include "packet_pool.h"

namespace network {


class NetPacketBuffer
{
public:
    uint64_t m_ulClientSeq;
    int32_t  m_iSockFd;

    int32_t  m_iProtoNo;
    int32_t  m_iBufferSize;
    size_t   m_dwCreateTime;

    ST_PacketMemory* m_pstMemory;

    void construct()
    {
        memset(this, 0, sizeof(*this));
    }

    NetPacketBuffer()
    {
        construct();
    }

    NetPacketBuffer(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize, ST_PacketMemory* pstMemory, BYTE* pchBuffer)
    {
        construct();

        m_ulClientSeq = ulClientSeq;
        m_iSockFd = iSockFd;
        m_iBufferSize = iBufferSize;
        m_iProtoNo = iProtoNo;
        m_dwCreateTime = GetMilliSecond();

        assert(pstMemory);
        m_pstMemory = pstMemory;
        m_pstMemory->Init(iBufferSize, pchBuffer);
    }

    NetPacketBuffer(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize)
    {
        construct();

        m_ulClientSeq = ulClientSeq;
        m_iSockFd = iSockFd;
        m_iBufferSize = iBufferSize;
        m_iProtoNo = iProtoNo;
        m_dwCreateTime = GetMilliSecond();

        m_pstMemory = NULL;
    }

    NetPacketBuffer(const NetPacketBuffer& rstPacketBuffer)
    {
        memcpy(this, &rstPacketBuffer, sizeof(rstPacketBuffer));
    }

    int32_t GetClientFd()
    {
        return m_iSockFd;
    }

    uint64_t GetClientSeq()
    {
        return m_ulClientSeq;
    }

    int32_t GetProtoNo()
    {
        return m_iProtoNo;
    }

    int32_t GetBufferSize()
    {
        return m_iBufferSize;
    }

    int32_t GetPacketSize()
    {
        return m_iBufferSize + NETWORK_PACKET_HEADER_SIZE;
    }

    size_t GetCreateTime()
    {
        return (size_t)m_dwCreateTime;
    }

    BYTE* GetBuffer()
    {
        assert(m_pstMemory);
        return m_pstMemory->GetBuffer();
    }

    ST_PacketMemory* GetMemory()
    {
        assert(m_pstMemory);
        return m_pstMemory;
    }

    void RequireMemory(int32_t iBufferSize, BYTE* pchBuffer)
	{
        if(NULL != m_pstMemory)
        {
            CPacketMemoryPool::Instance().ReleasePacketMemory(m_pstMemory);
        }
        ST_PacketMemory* pstMemory = CPacketMemoryPool::Instance().GetFreePacketMemory();;
        assert(pstMemory);

        m_pstMemory = pstMemory;
        m_pstMemory->Init(iBufferSize, pchBuffer);
	}

    void ReleaseMemory()
	{
        CPacketMemoryPool::Instance().ReleasePacketMemory(GetMemory());
	}

    void PrepareHeader(BYTE (&pchBuffer)[NETWORK_PACKET_HEADER_SIZE])
    {
        BYTE* ptr = pchBuffer;
		PackInt(ptr, (int16_t)(GetBufferSize() + NETWORK_PACKET_HEADER_SIZE + NETWORK_PACKET_TAIL_SIZE));
		PackInt(ptr, (int16_t)GetProtoNo());
    }

    void PrepareTail(BYTE (&pchBuffer)[NETWORK_PACKET_TAIL_SIZE])
    {
        BYTE* ptr = pchBuffer;
		PackInt(ptr, (size_t)GetMilliSecond());
    }

};

} // namespace network

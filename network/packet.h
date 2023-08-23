#pragma once

#include "common/utils/time.h"
#include "defines.h"
#include "unpack.h"

namespace network {

class NetPacketHeader
{
public:
    uint64_t m_ulClientSeq;
    int32_t  m_iSockFd;

    int32_t  m_iProtoNo;
    int32_t  m_iBufferSize;
    size_t   m_dwCreateTime;

    NetPacketHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    void Init(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize)
    {
        m_ulClientSeq = ulClientSeq;
        m_iSockFd = iSockFd;
        m_iBufferSize = iBufferSize;
        m_iProtoNo = iProtoNo;
        m_dwCreateTime = GetMilliSecond();
    }
};

class NetPacketBuffer
{
public:
    NetPacketHeader m_stHeader;
    BYTE   m_achBytesArray[NETWORK_PACKET_BUFFER_DEFAULT_SIZE];
    BYTE*  m_pchBytesExtraSpace;

    void construct()
    {
        memset(this, 0, sizeof(*this));
    }

    NetPacketBuffer()
    {
        construct();
    }

    NetPacketBuffer(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize, BYTE* pchBuffer)
    {
        construct();
        Init(ulClientSeq, iSockFd, iProtoNo, iBufferSize, pchBuffer);
    }

    NetPacketBuffer(const NetPacketBuffer& rstPacketBuffer)
    {
        memcpy(this, &rstPacketBuffer, sizeof(rstPacketBuffer));
    }

    ~NetPacketBuffer()
    {
        if(NULL != m_pchBytesExtraSpace)
        {
            delete[] m_pchBytesExtraSpace;
            m_pchBytesExtraSpace = NULL;
        }
        construct();
    }

    void Init(uint64_t ulClientSeq, int32_t iSockFd, int32_t iProtoNo, int32_t iBufferSize, BYTE* pchBuffer)
    {
        construct();
        m_stHeader.Init(ulClientSeq, iSockFd, iProtoNo, iBufferSize);

		if((size_t)iBufferSize > sizeof(m_achBytesArray))
		{
			m_pchBytesExtraSpace = new BYTE[iBufferSize];
			memcpy(m_pchBytesExtraSpace, pchBuffer, iBufferSize);
		}
		else
		{
			memcpy(m_achBytesArray, pchBuffer, iBufferSize);
		}
    }

    int32_t GetClientFd()
    {
        return m_stHeader.m_iSockFd;
    }

    uint64_t GetClientSeq()
    {
        return m_stHeader.m_ulClientSeq;
    }

    int32_t GetProtoNo()
    {
        return m_stHeader.m_iProtoNo;
    }

    int32_t GetBufferSize()
    {
        return m_stHeader.m_iBufferSize;
    }

    int32_t GetPacketSize()
    {
        return m_stHeader.m_iBufferSize + NETWORK_PACKET_HEADER_SIZE;
    }

    size_t GetCreateTime()
    {
        return (size_t)m_stHeader.m_dwCreateTime;
    }

    BYTE* GetBuffer()
    {
        if(NULL != m_pchBytesExtraSpace)
        {
            return m_pchBytesExtraSpace;
        }

        return m_achBytesArray;
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

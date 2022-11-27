#pragma once

#include "common/utils/time.h"
#include "defines.h"
#include "unpack.h"

namespace network {

class NetPacketHeader
{
public:
    int32_t  m_iSockFd;
    uint16_t m_wProtoNo;
    uint16_t m_wBufferSize;
    size_t   m_dwCreateTime;

    NetPacketHeader()
    {
        memset(this, 0, sizeof(*this));
    }

    void Init(int32_t iSockFd, uint16_t wProtoNo, uint16_t wBufferSize)
    {
        m_iSockFd = iSockFd;
        m_wBufferSize = wBufferSize;
        m_wProtoNo = wProtoNo;
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

    NetPacketBuffer(int32_t iSockFd, int16_t wProtoNo, int16_t wBufferSize, BYTE* pchBuffer)
    {
        construct();
        Init(iSockFd, wProtoNo, wBufferSize, pchBuffer);
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

    void Init(int32_t iSockFd, int16_t wProtoNo, int16_t wBufferSize, BYTE* pchBuffer)
    {
        construct();
        m_stHeader.Init(iSockFd, wProtoNo, wBufferSize);

		if((size_t)wBufferSize > sizeof(m_achBytesArray))
		{
			m_pchBytesExtraSpace = new BYTE[wBufferSize];
			memcpy(m_pchBytesExtraSpace, pchBuffer, wBufferSize);
		}
		else
		{
			memcpy(m_achBytesArray, pchBuffer, wBufferSize);
		}
    }

    int32_t GetClientFd()
    {
        return (int32_t)m_stHeader.m_iSockFd;
    }

    int32_t GetProtoNo()
    {
        return (int32_t)m_stHeader.m_wProtoNo;
    }

    int32_t GetBufferSize()
    {
        return (int32_t)m_stHeader.m_wBufferSize;
    }

    int32_t GetPacketSize()
    {
        return (int32_t)(m_stHeader.m_wBufferSize + NETWORK_PACKET_HEADER_SIZE);
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
		PackInt(pchBuffer, (int16_t)(GetBufferSize() + NETWORK_PACKET_HEADER_SIZE + NETWORK_PACKET_TAIL_SIZE));
		PackInt(pchBuffer + sizeof(int16_t), (int16_t)GetProtoNo());
    }

    void PrepareTail(BYTE (&pchBuffer)[NETWORK_PACKET_TAIL_SIZE])
    {
		PackInt(pchBuffer, (size_t)GetMilliSecond());
    }

};

} // namespace network

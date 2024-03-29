#pragma once
#include <stdio.h>
#include <stdlib.h>

#include "common/defines.h"
#include "common/noncopyable.h"
#include "common/logger.h"
#include "common/utils/time.h"
#include "common/utils/uuid.h"

#include "network/defines.h"
#include "network/packet.h"
#include "network/context.h"

namespace network {

class SocketClientCtx : public ClientContext
{
public:
	uint64_t 			   m_ulClientUniqSeq;

	ST_ClientContextBuffer m_stContextBuffer;
	struct socket_server*  m_pstSocketServer;

	SocketClientCtx(int32_t iClientFd, struct socket_server* pstServer):
		m_pstSocketServer(pstServer)
	{
		m_ulClientUniqSeq = SnowFlakeUUID::Instance().GenerateSeqID();
		m_stContextBuffer.construct();
		m_stContextBuffer.m_iClientFd = iClientFd;
	}

	~SocketClientCtx()
	{
		m_pstSocketServer = NULL;
	}

    void SetClientIP(char* pchIP, size_t dwSize)
    {
		memset(m_stContextBuffer.m_chClientIP, 0, sizeof(m_stContextBuffer.m_chClientIP));
		dwSize = dwSize > NETWORK_SERVER_IP_ADDRESS_LEN ? NETWORK_SERVER_IP_ADDRESS_LEN : dwSize;
        memcpy(m_stContextBuffer.m_chClientIP, pchIP, dwSize);
    }

	const int32_t GetClientFd()
	{
		return m_stContextBuffer.m_iClientFd;
	}

	const char* GetClientIP()
	{
		return m_stContextBuffer.m_chClientIP;
	}

	const uint64_t GetClientSeq()
	{
		return m_ulClientUniqSeq;
	}

	int32_t PacketBufferRead(BYTE* pchBuffer, int32_t iBufferNeedRead)
	{
		int32_t iLeftSpace = (int32_t)sizeof(m_stContextBuffer.m_chBuffer) - (int32_t) m_stContextBuffer.m_iBufferSize;
        if(iLeftSpace < iBufferNeedRead)
        {
            LOGFATAL("client<{}> has no free buffer space for read!", m_stContextBuffer.m_iClientFd);
            return -1;
        }

        memcpy(m_stContextBuffer.m_chBuffer + m_stContextBuffer.m_iBufferSize, pchBuffer, iBufferNeedRead);
        m_stContextBuffer.m_iBufferSize += iBufferNeedRead;
		return iBufferNeedRead;
	}

	size_t PacketBufferSend(NetPacketBuffer& rstPacket)
	{
		int32_t iClientFd = rstPacket.m_iSockFd;

		int32_t iSizeTotal = NETWORK_PACKET_HEADER_SIZE + rstPacket.GetBufferSize() + NETWORK_PACKET_TAIL_SIZE;
		BYTE* buffer = (BYTE*)malloc(iSizeTotal);

		BYTE chHeaderArray[NETWORK_PACKET_HEADER_SIZE] = {'\0'};
		BYTE chTailArray[NETWORK_PACKET_TAIL_SIZE] = {'\0'}; 		// 尾部增加一个时间戳
		rstPacket.PrepareHeader(chHeaderArray);
		rstPacket.PrepareTail(chTailArray);

		memcpy(buffer, chHeaderArray, NETWORK_PACKET_HEADER_SIZE);
		memcpy(buffer + NETWORK_PACKET_HEADER_SIZE, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
		memcpy(buffer + NETWORK_PACKET_HEADER_SIZE + rstPacket.GetBufferSize(), chTailArray, NETWORK_PACKET_TAIL_SIZE);

		socket_server_send(m_pstSocketServer, iClientFd, buffer, iSizeTotal);

		return iSizeTotal;
	}

	ST_ClientContextBuffer* GetContextBuffer()
	{
		return &m_stContextBuffer;
	}

};

} // namespace network

#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#include "common/defines.h"
#include "common/noncopyable.h"
#include "common/logger.h"
#include "common/utils/time.h"
#include "common/utils/uuid.h"

#include "network/defines.h"
#include "network/packet.h"
#include "network/context.h"


namespace network {


class LibeventClientCtx : public ClientContext
{
public:
	ST_ClientContextBuffer  m_stContextBuffer;
	struct bufferevent*		m_pstBufferEvent;

	LibeventClientCtx(evutil_socket_t iClientFd, struct bufferevent* pstBufferEvent, struct sockaddr_in& rstClientAddr):
		m_pstBufferEvent(pstBufferEvent)
	{
		m_stContextBuffer.construct();
		m_stContextBuffer.m_ulClientSeq = SnowFlakeUUID::Instance().GenerateSeqID();
		m_stContextBuffer.m_iClientFd = iClientFd;
		memset(m_stContextBuffer.m_chClientIP, 0, sizeof(m_stContextBuffer.m_chClientIP));
		snprintf(m_stContextBuffer.m_chClientIP, sizeof(m_stContextBuffer.m_chClientIP), "%s:%u",
				inet_ntoa(rstClientAddr.sin_addr), ntohs(rstClientAddr.sin_port));
	}

	~LibeventClientCtx()
	{
		if(NULL != m_pstBufferEvent)
		{
			bufferevent_free(m_pstBufferEvent);
		}
		m_pstBufferEvent = NULL;
	}

	const char* GetClientIP()
	{
		return m_stContextBuffer.m_chClientIP;
	}

	const uint64_t GetClientSeq()
	{
		return m_stContextBuffer.m_ulClientSeq;
	}

	const int32_t GetClientFd()
	{
		return m_stContextBuffer.m_iClientFd;
	}

	std::string Repr()
	{
		char chBuffer[128] = {0};
		sprintf(chBuffer, "Client<%ld><%d><%s>", m_stContextBuffer.m_ulClientSeq, m_stContextBuffer.m_iClientFd, m_stContextBuffer.m_chClientIP);
		return std::string(chBuffer);
	}

	int32_t PacketBufferRead(struct bufferevent* pstEvtobj)
	{
		int32_t iReadLen = 0;

		int32_t iBufferRecv = NETWORK_PACKET_BUFFER_READ_SIZE;
		int32_t iBufferSize = m_stContextBuffer.m_iBufferSize;
		iBufferRecv = (int32_t)sizeof(m_stContextBuffer.m_chBuffer) - (int32_t)iBufferSize;
		if(iBufferRecv <= 0)
		{
			LOGFATAL("client<{}> has no free buffer space for read! BufferSize: {}", m_stContextBuffer.m_iClientFd, iBufferSize);
			return -1;
		}

		iReadLen = bufferevent_read(pstEvtobj, m_stContextBuffer.m_chBuffer + iBufferSize, iBufferRecv);
		m_stContextBuffer.m_iBufferSize += iReadLen;

		return iReadLen;
	}

	size_t PacketBufferSend(NetPacketBuffer& rstPacket)
	{
		assert(m_pstBufferEvent);

		BYTE chHeaderArray[NETWORK_PACKET_HEADER_SIZE] = {'\0'};
		BYTE chTailArray[NETWORK_PACKET_TAIL_SIZE] = {'\0'}; 		// 尾部增加一个时间戳
		rstPacket.PrepareHeader(chHeaderArray);
		rstPacket.PrepareTail(chTailArray);

		bufferevent_write(m_pstBufferEvent, chHeaderArray, sizeof(chHeaderArray));
		bufferevent_write(m_pstBufferEvent, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
		bufferevent_write(m_pstBufferEvent, chTailArray, sizeof(chTailArray));

		return (NETWORK_PACKET_HEADER_SIZE + rstPacket.GetBufferSize() + NETWORK_PACKET_TAIL_SIZE);
	}

	ST_ClientContextBuffer* GetContextBuffer()
	{
		return &m_stContextBuffer;
	}

};

} // namespace network

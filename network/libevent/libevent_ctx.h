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

#include "network/defines.h"
#include "network/packet.h"
#include "network/context.h"


namespace network {


class LibeventClientCtx : public ClientContext
{
public:
	ST_ClientContextBuffer  m_stContextBuffer;
	struct bufferevent*		m_pstBufferEvent;

	LibeventClientCtx()
	{
		_construct();
	}

	~LibeventClientCtx()
	{
		if(NULL != m_pstBufferEvent)
		{
			bufferevent_free(m_pstBufferEvent);
		}
		_construct();
	}

    void SetClientIP(struct sockaddr_in& rstClientAddr)
    {
		memset(m_stContextBuffer.m_chClientIP, 0, sizeof(m_stContextBuffer.m_chClientIP));
		snprintf(m_stContextBuffer.m_chClientIP, sizeof(m_stContextBuffer.m_chClientIP), "%s:%u", inet_ntoa(rstClientAddr.sin_addr), ntohs(rstClientAddr.sin_port));
    }

	void Init(evutil_socket_t iClientFd, struct bufferevent* pstBufferEvent)
	{
		m_stContextBuffer.m_iClientFd = iClientFd;
		m_pstBufferEvent = pstBufferEvent;
	}

	int32_t GetClientFd()
	{
		return m_stContextBuffer.m_iClientFd;
	}

	int32_t ReadToRecvBuffer(struct bufferevent* pstEvtobj)
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

	size_t ExecuteCmdSendOnePacket(NetPacketBuffer& rstPacket)
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

private:

	void _construct()
	{
		m_stContextBuffer.construct();
		m_pstBufferEvent = NULL;
	}

};

} // namespace network

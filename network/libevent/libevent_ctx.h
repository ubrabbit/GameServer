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
#include "network/pool.h"


namespace network {


class LibeventClientCtx
{
public:

	evutil_socket_t 		m_iClientFd;
	struct bufferevent*		m_pstBufferEvent;

	ServerNetBufferCtx* 	m_pstServerCtx;

	int32_t m_iBufferSize;
	BYTE    m_chBuffer[NETWORK_PACKET_BUFFER_READ_SIZE * 10];

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
	}

	void Init(evutil_socket_t iClientFd, struct bufferevent* pstBufferEvent, ServerNetBufferCtx* pstServerCtx)
	{
		m_iClientFd = iClientFd;
		m_pstBufferEvent = pstBufferEvent;
		m_pstServerCtx = pstServerCtx;
	}

	size_t ReadToRecvBuffer(struct bufferevent* pstEvtobj)
	{
		int32_t iReadLen = 0;
		do{
			int32_t iBufferRecv = NETWORK_PACKET_BUFFER_READ_SIZE;
			iBufferRecv = (int32_t)sizeof(m_chBuffer) - (int32_t)m_iBufferSize;
			if(iBufferRecv <= 0)
			{
				LOGFATAL("client<{}> has no free buffer space for read!", m_iClientFd);
				return -1;
			}

			iReadLen = bufferevent_read(pstEvtobj, m_chBuffer + m_iBufferSize, iBufferRecv);
			m_iBufferSize += iReadLen;
		}while(iReadLen > 0);

		return 0;
	}

    int32_t UnpackPacketFromRecvBuffer(ServerNetBufferCtx& rstServerCtx)
	{
		size_t dwStartTime = GetMilliSecond();
		int32_t iTotalPackets = 0;

		int32_t iLeftSize = m_iBufferSize;
		BYTE* pchBufferPtr = m_chBuffer;
		while(iLeftSize >= NETWORK_PACKET_HEADER_SIZE)
		{
			int32_t iPacketSize = (int32_t)UnpackInt(pchBufferPtr, (int16_t)sizeof(int16_t));
			// 还没收到足够长度的包
			if(iLeftSize < iPacketSize || iPacketSize < NETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			int32_t iProtoNo = (int32_t)UnpackInt(pchBufferPtr + sizeof(int16_t), (int16_t)sizeof(int16_t));
			int16_t wBodySize = (int16_t)(iPacketSize - NETWORK_PACKET_HEADER_SIZE);
			if(wBodySize <= 0)
			{
				LOGERROR("client<{}> recv packet error by body size {} not in valid range.", wBodySize);
				return -2;
			}

			BYTE* pstBufferData = pchBufferPtr + NETWORK_PACKET_HEADER_SIZE;
			NetPacketBuffer* pstPacket = rstServerCtx.RequireNetPacketSlot(m_iClientFd, (int16_t)iProtoNo, (int16_t)wBodySize, pstBufferData);
			if(NULL == pstPacket)
			{
				LOGFATAL("client<{}> recv packet but net buffer is full!.", m_iClientFd);
				return -1;
			}

			pchBufferPtr += iPacketSize;
			iLeftSize -= iPacketSize;
			iTotalPackets++;
		}

		iLeftSize = iLeftSize > 0 ? iLeftSize : 0;
		if(iLeftSize > 0)
		{
			//把剩余的缓存数据往前移
			memmove(m_chBuffer, pchBufferPtr, iLeftSize);
		}
		m_iBufferSize = iLeftSize;

		size_t dwCostTime = GetMilliSecond() - dwStartTime;
		if(iTotalPackets > 0)
		{
			LOGINFO("UnpackPacketFromRecvBuffer Finished: TotalPackets: {} Cost: {} ms", iTotalPackets, dwCostTime);
		}
		return 0;
	}

private:

	void _construct()
	{
		memset(this, 0, sizeof(*this));
	}
};

} // namespace network

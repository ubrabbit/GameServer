#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>

#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>

#include "common/defines.h"
#include "common/logger.h"
#include "network/defines.h"
#include "network/pool.h"
#include "libevent_ctx.h"

namespace network {

typedef struct ST_LibeventEventHandle
{
	struct event_base* 		m_pstEventBase;
	ServerNetBufferCtx* 	m_pstServerCtx;

	ST_LibeventEventHandle()
	{
		memset(this, 0, sizeof(*this));
	}
}ST_LibeventEventHandle;

void OnFatalError(int iError);

void OnListenerAcceptConnect(struct evconnlistener* pstListener, evutil_socket_t iClientFd,
				struct sockaddr* pstSockAddr, socklen_t iSockLen, void* pstVoidEventHandle);

void OnListenerAcceptError(struct evconnlistener* pstListener, void* pstVoidEventHandle);

void OnRecvINTSignal(evutil_socket_t fd, short event, void* pstVoidSignal);

void OnBufferEventWrite(struct bufferevent* pstEvtobj, void* pstVoidClientCtx);

void OnBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidClientCtx);

void OnBufferEventTrigger(struct bufferevent* pstEvtobj, short wEvent, void* pstVoidClientCtx);

void CloseClient(LibeventClientCtx* pstClientCtx);

} // namespace network

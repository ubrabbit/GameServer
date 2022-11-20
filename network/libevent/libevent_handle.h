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
#include "network/context.h"
#include "libevent_ctx.h"

namespace network {

void OnFatalError(int iError);

void OnListenerAcceptConnect(struct evconnlistener* pstListener, evutil_socket_t iClientFd,
				struct sockaddr* pstSockAddr, socklen_t iSockLen, void* pstVoidServerCtx);

void OnListenerAcceptError(struct evconnlistener* pstListener, void* pstVoidServerCtx);

void OnRecvINTSignal(evutil_socket_t fd, short event, void* pstVoidSignal);

void OnBufferEventWrite(struct bufferevent* pstEvtobj, void* pstVoidServerCtx);

void OnBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidServerCtx);

void OnBufferEventTrigger(struct bufferevent* pstEvtobj, short wEvent, void* pstVoidServerCtx);

void CloseClient(ServerContext& rstServerCtx, LibeventClientCtx& rstClientCtx);

} // namespace network

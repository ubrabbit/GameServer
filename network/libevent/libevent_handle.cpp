#include "libevent_handle.h"


namespace network {

static inline int _SetSocketOpt(int iClientFd) {
	struct linger stLinger = {1,0};
	int iResult = setsockopt(iClientFd, SOL_SOCKET, SO_LINGER, &stLinger, sizeof(stLinger));
	if(0 != iResult)
	{
		return iResult;
	}

	int iKeepalive = 1;
	iResult = setsockopt(iClientFd, SOL_SOCKET, SO_KEEPALIVE, &iKeepalive, sizeof(iKeepalive));
	if(0 != iResult)
	{
		return iResult;
	}
	return iResult;
}

void OnFatalError(int iError)
{
    LOGCRITICAL("libevent fatal error: {}", strerror(iError));
}

void OnListenerAcceptConnect(struct evconnlistener* pstListener, evutil_socket_t iClientFd,
				             struct sockaddr* pstSockAddr, socklen_t iSockLen, void* pstVoidServerCtx)
{
    LOGINFO("recv new connect, sockfd<{}>", (int)iClientFd);

	struct event_base* pstEvtBase = evconnlistener_get_base(pstListener);
	assert(pstEvtBase);

	struct bufferevent *pstEvtObj = bufferevent_socket_new(pstEvtBase, iClientFd, BEV_OPT_CLOSE_ON_FREE);
	if(NULL == pstEvtObj){
		LOGFATAL("bufferevent_socket_new error!");
		event_base_loopbreak(pstEvtBase);
		return;
	}

	int iOptResult = _SetSocketOpt(iClientFd);
	if(0 != iOptResult)
	{
		LOGFATAL("client<{}> setsockopt error: {}", iClientFd, iOptResult);
		return;
	}

	// 设置单次读写包的大小
	bufferevent_set_max_single_read(pstEvtObj, NETWORK_PACKET_BUFFER_READ_SIZE);
	bufferevent_set_max_single_write(pstEvtObj, NETWORK_PACKET_BUFFER_READ_SIZE);

	ServerContext* pstServerCtx = (ServerContext*)pstVoidServerCtx;
	assert(pstServerCtx);

	LibeventClientCtx* pstClientCtx = new LibeventClientCtx();
	pstClientCtx->Init(iClientFd, pstEvtObj, pstServerCtx);

	pstServerCtx->m_stClientPool.AddClient((int)iClientFd, (void*)pstClientCtx);

	bufferevent_setcb(pstEvtObj, OnBufferEventRead, OnBufferEventWrite, OnBufferEventTrigger, pstClientCtx);
	//EV_PERSIST可以让注册的事件在执行完后不被删除,直到调用event_del()删除
	bufferevent_enable(pstEvtObj, EV_READ|EV_WRITE|EV_PERSIST);
}

void OnListenerAcceptError(struct evconnlistener* pstListener, void* pstVoidEventHandle)
{
	evutil_socket_t iListenFd = evconnlistener_get_fd(pstListener);
	int iError = EVUTIL_SOCKET_ERROR();
	LOGERROR("listener {} accept error: {}({})", (int)iListenFd, iError, evutil_socket_error_to_string(iError));

	struct event_base* pstEvtBase = evconnlistener_get_base(pstListener);
	assert(pstEvtBase);

	event_base_loopexit(pstEvtBase, NULL);
}

void OnBufferEventWrite(struct bufferevent* pstEvtobj, void* pstVoidClientCtx)
{
	LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)pstVoidClientCtx;
}

void OnBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidClientCtx)
{
	evutil_socket_t	iClientFd = bufferevent_getfd(pstEvtobj);

	LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)pstVoidClientCtx;
	assert(pstClientCtx);

	int32_t iRet = 0;
	iRet = pstClientCtx->ReadToRecvBuffer(pstEvtobj);
	if(0 != iRet)
	{
		CloseClient(pstClientCtx);
		return;
	}

	ServerContext* pstServerCtx = pstClientCtx->m_pstServerCtx;
	assert(pstServerCtx);

	iRet = pstClientCtx->UnpackPacketFromRecvBuffer(*pstServerCtx);
	if(0 != iRet)
	{
		CloseClient(pstClientCtx);
		return;
	}

}

void OnBufferEventTrigger(struct bufferevent* pstEvtobj, short wEvent, void* pstVoidClientCtx)
{
    evutil_socket_t	iSockFd = bufferevent_getfd(pstEvtobj);

	bool bEventFree = false;
	if(wEvent & BEV_EVENT_TIMEOUT){
		LOGINFO("client<{}> socket timeout.", (int32_t)iSockFd);
		bEventFree = true;
	}
	if(wEvent & BEV_EVENT_EOF){
        LOGINFO("client<{}> connection closed.", (int32_t)iSockFd);
		bEventFree = true;
	}
	if(wEvent & BEV_EVENT_ERROR){
        LOGINFO("client<{}> socket error.", (int32_t)iSockFd);
		bEventFree = true;
	}

	if(bEventFree)
	{
		LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)pstVoidClientCtx;
		int32_t iErrCode = evutil_socket_geterror(iSockFd);
		LOGINFO("client<{}> closed by error<{}><{}>", (int32_t)iSockFd, iErrCode, evutil_socket_error_to_string(iErrCode));
		CloseClient(pstClientCtx);
	}
}


void CloseClient(LibeventClientCtx* pstClientCtx)
{
	if(NULL == pstClientCtx)
	{
		LOGERROR("clost client buf ptr is NULL!");
		return;
	}

	LOGINFO("client<{}> closed.", (int32_t)pstClientCtx->m_iClientFd);

	ServerContext* pstServerCtx = pstClientCtx->m_pstServerCtx;
	assert(pstServerCtx);

	pstServerCtx->m_stClientPool.RemoveClient((int32_t)pstClientCtx->m_iClientFd);
	delete pstClientCtx;
}

} // namespace network

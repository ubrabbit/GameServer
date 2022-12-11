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
	struct event_base* pstEvtBase = evconnlistener_get_base(pstListener);
	assert(pstEvtBase);

	struct sockaddr_in stClientAddr;
	socklen_t dwAddrLen = sizeof(stClientAddr);
	getpeername(iClientFd, (struct sockaddr*)&stClientAddr, &dwAddrLen);
	LOGINFO("client<{}><{}:{}> accept connect.", iClientFd, inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port));

	ServerContext* pstServerCtx = (ServerContext*)pstVoidServerCtx;
	assert(pstServerCtx);

	LibeventClientCtx* pstLastClientCtx = (LibeventClientCtx*)(pstServerCtx->m_stClientPool.GetClientByFd(iClientFd));
	if(NULL != pstLastClientCtx)
	{
		LOGFATAL("client<{}><{}:{}> accept connect but sockfd already exists in pool!",
				iClientFd, inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port));
		CloseClient(*pstServerCtx, *pstLastClientCtx);
	}

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

	LibeventClientCtx* pstClientCtx = new LibeventClientCtx(iClientFd, pstEvtObj, stClientAddr);
	pstServerCtx->m_stClientPool.AddClient(pstClientCtx->GetClientSeq(), pstClientCtx);
	LOGINFO("{} accept connect success.", pstClientCtx->Repr());

	bufferevent_setcb(pstEvtObj, OnBufferEventRead, OnBufferEventWrite, OnBufferEventTrigger, pstServerCtx);
	//EV_PERSIST可以让注册的事件在执行完后不被删除,直到调用event_del()删除
	bufferevent_enable(pstEvtObj, EV_READ|EV_WRITE|EV_PERSIST);
}

void OnListenerAcceptError(struct evconnlistener* pstListener, void* pstVoidServerCtx)
{
	evutil_socket_t iListenFd = evconnlistener_get_fd(pstListener);
	int iError = EVUTIL_SOCKET_ERROR();

	struct sockaddr_in stClientAddr;
	socklen_t dwAddrLen = sizeof(stClientAddr);
	getpeername(iListenFd, (struct sockaddr*)&stClientAddr, &dwAddrLen);
	LOGERROR("client<{}><{}:{}> accept error: {}({})", (int)iListenFd,
			inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port),
			iError, evutil_socket_error_to_string(iError));

	struct event_base* pstEvtBase = evconnlistener_get_base(pstListener);
	assert(pstEvtBase);
	event_base_loopexit(pstEvtBase, NULL);
}

void OnBufferEventWrite(struct bufferevent* pstEvtobj, void* pstVoidServerCtx)
{
}

void OnBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidServerCtx)
{
	evutil_socket_t	iClientFd = bufferevent_getfd(pstEvtobj);
	ServerContext* pstServerCtx = (ServerContext*)pstVoidServerCtx;
	assert(pstServerCtx);
	ServerContext& rstServerCtx = *pstServerCtx;

	LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)(rstServerCtx.m_stClientPool.GetClientByFd(iClientFd));
	if(NULL == pstClientCtx)
	{
		struct sockaddr_in stClientAddr;
		socklen_t dwAddrLen = sizeof(stClientAddr);
		getpeername(iClientFd, (struct sockaddr*)&stClientAddr, &dwAddrLen);
		int32_t iErrCode = evutil_socket_geterror(iClientFd);
		LOGERROR("client<{}><{}:{}> read error: {}({})", (int)iClientFd,
				inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port),
				iErrCode, evutil_socket_error_to_string(iErrCode));
		return;
	}

	int32_t iReadLen = 0;
	do{
		iReadLen = pstClientCtx->PacketBufferRead(pstEvtobj);
		int32_t iRet = rstServerCtx.ExecuteCmdRecvAllPacket(pstClientCtx);
		if(0 != iRet)
		{
			CloseClient(rstServerCtx, *pstClientCtx);
			return;
		}
		//第一次read缓冲区可能满了，正常情况下会解包空出空间，所以再收一次
		iReadLen = pstClientCtx->PacketBufferRead(pstEvtobj);
	}while(iReadLen > 0);

}

void OnBufferEventTrigger(struct bufferevent* pstEvtobj, short wEvent, void* pstVoidServerCtx)
{
	evutil_socket_t	iClientFd = bufferevent_getfd(pstEvtobj);
	ServerContext* pstServerCtx = (ServerContext*)pstVoidServerCtx;
	assert(pstServerCtx);
	ServerContext& rstServerCtx = *pstServerCtx;

	struct sockaddr_in stClientAddr;
	socklen_t dwAddrLen = sizeof(stClientAddr);
	getpeername(iClientFd, (struct sockaddr*)&stClientAddr, &dwAddrLen);

	LibeventClientCtx* pstClientCtx = (LibeventClientCtx*)rstServerCtx.m_stClientPool.GetClientByFd(iClientFd);
	if(NULL == pstClientCtx)
	{
		int32_t iErrCode = evutil_socket_geterror(iClientFd);
		LOGERROR("client<{}><{}:{}> read error: {}({})", (int)iClientFd,
				inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port),
				iErrCode, evutil_socket_error_to_string(iErrCode));
		return;
	}

	bool bEventFree = false;
	if(wEvent & BEV_EVENT_TIMEOUT){
		bEventFree = true;
	}
	if(wEvent & BEV_EVENT_EOF){
		bEventFree = true;
	}
	if(wEvent & BEV_EVENT_ERROR){
		bEventFree = true;
	}

	if(bEventFree)
	{
		int32_t iErrCode = evutil_socket_geterror(iClientFd);
		LOGINFO("client<{}><{}:{}> closed by event<{}> error<{}><{}>", (int32_t)iClientFd,
				inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port),
				wEvent, iErrCode, evutil_socket_error_to_string(iErrCode));
		CloseClient(rstServerCtx, *pstClientCtx);
	}
	else
	{
		int32_t iErrCode = evutil_socket_geterror(iClientFd);
		LOGINFO("client<{}><{}:{}> occur event<{}> error <{}><{}>", (int32_t)iClientFd,
				inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port),
				wEvent, iErrCode, evutil_socket_error_to_string(iErrCode));
	}
}


void CloseClient(ServerContext& rstServerCtx, LibeventClientCtx& rstClientCtx)
{
	LOGINFO("client<{}><{}> closed.", rstClientCtx.GetClientFd(), rstClientCtx.GetClientSeq());
	rstServerCtx.m_stClientPool.RemoveClient(rstClientCtx.GetClientSeq());

	LibeventClientCtx* pstClientCtx = &rstClientCtx;
	delete pstClientCtx;
}

} // namespace network

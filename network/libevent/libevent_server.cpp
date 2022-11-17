#include "common/logger.h"

#include "libevent_server.h"
#include "libevent_handle.h"
#include "libevent_ctx.h"


namespace network {

LibeventServer::LibeventServer()
{
    _InitEventMgr();
}

LibeventServer::~LibeventServer()
{
    _ReleaseEventMgr();
}

void LibeventServer::_InitEventMgr()
{
    m_pstEventBase = NULL;
    m_pstListener = NULL;
}

void LibeventServer::_ReleaseEventMgr()
{
    if(NULL != m_pstEventBase)
    {
        event_base_free(m_pstEventBase);
    }

    if(NULL != m_pstListener)
    {
        evconnlistener_free(m_pstListener);
    }

    _InitEventMgr();
}

void LibeventServer::PrintSupportMethod()
{
    assert(m_pstEventBase);
    LOGINFO("current function: {}", event_base_get_method(m_pstEventBase));

    enum event_method_feature eEventMask = (enum event_method_feature)event_base_get_features(m_pstEventBase);
	/*
	EV_FEATURE_ET	:	要求支持边沿触发的后端
	EV_FEATURE_O1	:	要求添加、删除单个事件,或者确定哪个事件激活的操作是 O(1)复杂度的后端
	EV_FEATURE_FDS	:	要求支持任意文件描述符,而不仅仅是套接字的后端
	*/
	if (eEventMask & EV_FEATURE_ET)
    {
	    LOGINFO("edge-triggered events are supported.");
    }
	if (eEventMask & EV_FEATURE_O1)
    {
	    LOGINFO("O(1) event notification is supported.");
    }
	if (eEventMask & EV_FEATURE_FDS)
    {
	    LOGINFO("all fd types are supported.");
    }
}

bool LibeventServer::_InitEventBase(ServerContext& rstServerCtx, int32_t iRecvCtrlFd)
{
	//if use multi_threads ,must call this before any event created.
	evthread_use_pthreads();

    //设置致命错误发生后的回调函数
    event_set_fatal_callback(OnFatalError);

	struct event_config* pstEventConfig = event_config_new();
    if(NULL == pstEventConfig)
    {
        LOGCRITICAL("event_config_new fail!");
        return false;
    }

	//do not use select&poll
	event_config_avoid_method(pstEventConfig, "select");
	event_config_avoid_method(pstEventConfig, "poll");

    struct event_base* pstEventBase = event_base_new_with_config(pstEventConfig);
    event_config_free(pstEventConfig);
    if(NULL == pstEventBase)
    {
        LOGCRITICAL("event_base_new_with_config fail!");
        return false;
    }

    m_stEventHandle.m_pstEventBase = pstEventBase;
    m_stEventHandle.m_pstServerCtx = &rstServerCtx;
    struct event* pstNtfEvt = event_new(pstEventBase, iRecvCtrlFd, EV_READ|EV_PERSIST, OnRecvNetCmdCtrl, &m_stEventHandle);
    if(NULL == pstNtfEvt)
    {
        LOGCRITICAL("create ntf event fail!");
        return false;
    }
	if(event_add(pstNtfEvt, NULL)<0){
		LOGCRITICAL("add ntf event error!");
        return false;
	}

    m_pstEventBase = pstEventBase;
    PrintSupportMethod();
    return true;
}

bool LibeventServer::_InitListener(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx)
{
    LOGINFO("start listern {}:{}", rstServerIPInfo.m_achServerIP, rstServerIPInfo.m_wServerPort);

    struct sockaddr_in stSockAddrIn;
	memset(&stSockAddrIn, 0, sizeof(struct sockaddr_in));
	stSockAddrIn.sin_family = AF_INET;
	stSockAddrIn.sin_addr.s_addr = inet_addr(rstServerIPInfo.m_achServerIP);
	stSockAddrIn.sin_port = htons((unsigned short)rstServerIPInfo.m_wServerPort);

	struct evconnlistener* pstListener = evconnlistener_new_bind(m_pstEventBase,
            (evconnlistener_cb)OnListenerAcceptConnect,
            &rstServerCtx,
			LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
            NETWORK_SERVER_LISTEN_QUEUE_LEN,
			(struct sockaddr *)&stSockAddrIn, sizeof(stSockAddrIn));
	if(NULL == pstListener){
		LOGFATAL("evconnlistener_new_bind create socket error!");
        return false;
	}
    m_pstListener = pstListener;
    evconnlistener_set_error_cb(m_pstListener, OnListenerAcceptError);
    return true;
}

void LibeventServer::Init(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx, int32_t iRecvCtrlFd)
{
    if(false == _InitEventBase(rstServerCtx, iRecvCtrlFd))
    {
        LOGCRITICAL("InitEventBase failed!");
        return;
    }

    if(false == _InitListener(rstServerIPInfo, rstServerCtx))
    {
        LOGCRITICAL("_InitListener failed!");
        return;
    }

    LOGINFO("init server success.");
    return;
}

void LibeventServer::StartEventLoop()
{
    assert(m_pstEventBase);

    //程序进入无限循环，等待就绪事件并执行事件处理
    LOGINFO("start eventLoop.");
    event_base_dispatch(m_pstEventBase);
    LOGINFO("finished eventLoop.");

    _ReleaseEventMgr();
}

} // namespace network

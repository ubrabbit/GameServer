#pragma once

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <list>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "common/defines.h"
#include "network/defines.h"
#include "network/context.h"
#include "libevent_handle.h"
#include "libevent_ctx.h"


namespace network
{

class LibeventServer : public NertworkServer
{
public:
    int32_t                 m_iRecvCtrlFd;
    int32_t                 m_iSendCtrlFd;

    LibeventServer();
    ~LibeventServer();

    void Init(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx);
    void StartEventLoop();
    void SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd);
    void RecvNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd);

    void PrintSupportMethod();

private:
    bool _InitEventBase(ServerContext& rstServerCtx, int32_t iRecvCtrlFd);
    bool _InitListener(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx);

    void _InitEventMgr();
    void _ReleaseEventMgr();

    ServerContext*          m_pstServerCtx;
    struct event_base*      m_pstEventBase;
    struct evconnlistener*  m_pstListener;

};

void OnRecvNetCmdCtrl(int32_t iRecvCtrlFd, short events, void* pstVoidServer);

} // namespace network

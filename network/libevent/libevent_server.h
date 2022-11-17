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
#include "network/pool.h"
#include "libevent_handle.h"
#include "libevent_cmd.h"
#include "libevent_ctx.h"


namespace network {

class LibeventServer
{
public:
    ST_LibeventEventHandle m_stEventHandle;

    LibeventServer();
    ~LibeventServer();

    void Init(struct ST_ServerIPInfo& rstServerIPInfo, ServerNetBufferCtx& rstServerCtx, int32_t iRecvCtrlFd);
    void StartEventLoop();

    void PrintSupportMethod();

private:
    bool _InitEventBase(ServerNetBufferCtx& rstServerCtx, int32_t iRecvCtrlFd);
    bool _InitListener(struct ST_ServerIPInfo& rstServerIPInfo, ServerNetBufferCtx& rstServerCtx);

    void _InitEventMgr();
    void _ReleaseEventMgr();

    struct event_base*      m_pstEventBase;
    struct evconnlistener*  m_pstListener;

};

} // namespace network

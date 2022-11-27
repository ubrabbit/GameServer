#pragma once

#include "common/logger.h"
#include "common/singleton.h"
#include "common/noncopyable.h"
#include "common/conf/config.h"
#include "common/utils/spinlock.h"
#include "common/utils/posix.h"
#include "network/defines.h"
#include "network/unpack.h"
#include "network/context.h"
#include "network/queue.h"
#include "network/libevent/libevent_server.h"
#include "network/socket/server_logic.h"


namespace network {

class ServerMonitor : public Singleton<ServerMonitor>
{
public:
    bool m_bInitSucc;

    bool Inited()
    {
        m_bInitSucc = true;
        return m_bInitSucc;
    }

    bool IsInited()
    {
        return m_bInitSucc;
    }
};

class Server : public noncopyable
{
public:

    NertworkServer*         m_pstServer;
    ServerContext           m_stServerCtx;

    bool StartServer(struct ST_ServerIPInfo& rstServerIPInfo);

    bool StartLibeventServer(struct ST_ServerIPInfo& rstServerIPInfo);
    bool StartSocketServer(struct ST_ServerIPInfo& rstServerIPInfo);

    void SendNetworkPackets();

    template<class T>
    void StartEventLoop(T& rstServer);

};

} // namespace network

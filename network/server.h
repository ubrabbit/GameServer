#pragma once

#include "common/logger.h"
#include "common/singleton.h"
#include "common/conf/config.h"
#include "common/utils/spinlock.h"
#include "network/defines.h"
#include "network/unpack.h"
#include "network/context.h"
#include "network/queue.h"
#include "network/libevent/libevent_server.h"


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

class Server
{
public:
    int32_t                 m_iRecvCtrlFd;
    int32_t                 m_iSendCtrlFd;

    LibeventServer          m_stLibeventServer;

    ServerContext           m_stServerCtx;

    bool StartServer(struct ST_ServerIPInfo& rstServerIPInfo);

    void SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd);

private:
    void _InitPipeFd(int32_t& iFdRead, int32_t& iFdWrite);

};

} // namespace network

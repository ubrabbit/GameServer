#pragma once
#include <pthread.h>

#include "common/logger.h"
#include "network/defines.h"
#include "network/context.h"
#include "network/packet.h"
#include "network/unpack.h"

extern "C"
{
#include "socket_server.h"
}
#include "socket_ctx.h"

namespace network
{

#define NETWORK_SOCKET_SERVER_OPAQUE (1001)


class SocketServer  : public NertworkServer
{
public:
    int32_t                 m_iRecvCtrlFd;
    int32_t                 m_iSendCtrlFd;

    struct socket_server*   m_pstSocketServer;
    ServerContext*          m_pstServerCtx;

    SocketServer();
    ~SocketServer();

    void Init(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx);
    void StartEventLoop();

    void SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd);
    void RecvNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd);

    void construct();

};

} // namespace network

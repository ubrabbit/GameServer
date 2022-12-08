#include "server.h"
#include "defines.h"
#include "common/utils/posix.h"

namespace network {


bool Server::StartLibeventServer(struct ST_ServerIPInfo& rstServerIPInfo)
{
    LOGINFO("StartLibeventServer");

    LibeventServer* pstLibEventServer = new LibeventServer();
    pstLibEventServer->Init(rstServerIPInfo, m_stServerCtx);
    ServerMonitor::Instance().Inited();

    m_pstServer = pstLibEventServer;
    StartEventLoop(*pstLibEventServer);
    return true;
}


bool Server::StartSocketServer(struct ST_ServerIPInfo& rstServerIPInfo)
{
    LOGINFO("StartSocketServer");

    SocketServer* pstSocketServer = new SocketServer();
    pstSocketServer->Init(rstServerIPInfo, m_stServerCtx);
    ServerMonitor::Instance().Inited();

    m_pstServer = pstSocketServer;
    StartEventLoop(*pstSocketServer);
    return true;
}


bool Server::StartServer(struct ST_ServerIPInfo& rstServerIPInfo)
{
    #ifdef _NETWORK_NET_MODE_SELECTED_SOCKET
        return StartSocketServer(rstServerIPInfo);
    #else
        return StartLibeventServer(rstServerIPInfo);
    #endif
}


void Server::SendNetworkPackets()
{
    assert(m_pstServer);

    struct ST_NETWORK_CMD_REQUEST stNetCmd(NETWORK_CMD_REQUEST_SEND_ALL_PACKET);
    m_pstServer->SendNetworkCmd(stNetCmd);
}


template<class T>
void Server::StartEventLoop(T& rstServerHandle)
{
    rstServerHandle.StartEventLoop();
}

} // namespace network

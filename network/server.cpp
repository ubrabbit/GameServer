#include "server.h"
#include "defines.h"

namespace network {

void Server::_InitPipeFd(int32_t& iFdRead, int32_t& iFdWrite)
{
    int iPipeFd[2];
	if(pipe(iPipeFd))
    {
        LOGCRITICAL("create recv/send pipe failed.");
		return;
	}
    iFdRead = iPipeFd[0];
    iFdWrite = iPipeFd[1];
}

bool Server::StartServer(struct ST_ServerIPInfo& rstServerIPInfo)
{
    _InitPipeFd(m_iRecvCtrlFd, m_iSendCtrlFd);
    m_stLibeventServer.Init(rstServerIPInfo, m_stServerRecvCtx, m_iRecvCtrlFd);
    ServerMonitor::Instance().Inited();

    m_stLibeventServer.StartEventLoop();
    return true;
}

void Server::SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
{
    if(false == m_stServerRecvCtx.HasPacketToSend())
    {
        return;
    }
    for(;;)
    {
        const char* pchRequest = (const char*)&rstNetCmd;
        ssize_t n = write(m_iSendCtrlFd, pchRequest, sizeof(rstNetCmd));
        if(n<0) {
            if (errno != EINTR) {
                LOGERROR("send ctrl command {} error: {}", rstNetCmd.m_Header, strerror(errno));
            }
            continue;
        }
        assert(n == sizeof(rstNetCmd));
        //LOGINFO("send net cmd [{}] success", rstNetCmd.m_Header);
        return;
    }
}

} // namespace network


#include "server_logic.h"

namespace network
{

static pthread_t g_SocketThreadPool[2];

inline void _ProcessNewClient(struct socket_server* pstServer, ServerContext& rstServerCtx, int32_t iClientFd, struct socket_message& rstMessage)
{
	LOGINFO("client<{}> connected.", iClientFd);
	size_t dwSize = strlen(rstMessage.data);
	SocketClientCtx* pstClient = (SocketClientCtx*)rstServerCtx.m_stClientPool.GetClientByFd(iClientFd);
	if(NULL == pstClient)
	{
		pstClient = new SocketClientCtx(iClientFd, pstServer);
		pstClient->SetClientIP(rstMessage.data, dwSize);
		rstServerCtx.m_stClientPool.AddClient(pstClient->GetClientSeq(), pstClient);
	}
	pstClient->SetClientIP(rstMessage.data, dwSize);
}

inline bool _ProcessCloseClient(struct socket_server* pstServer, ServerContext& rstServerCtx, int32_t iClientFd)
{
	LOGINFO("client<{}> closed.", iClientFd);
	SocketClientCtx* pstClient = (SocketClientCtx*)rstServerCtx.m_stClientPool.GetClientByFd(iClientFd);
	if(NULL == pstClient)
	{
		LOGINFO("client<{}> closed but not in client pool.", iClientFd);
		return false;
	}

	rstServerCtx.m_stClientPool.RemoveClient(pstClient->GetClientSeq());
	delete pstClient;

	return true;
}

/*
禁止在这个函数所处的线程调用任意的触发send_request的逻辑，这个线程只能转发数据到其他线程，通过mq!
*/
inline void _ProcessMessageData(struct socket_server* pstServer, ServerContext& rstServerCtx, int32_t iClientFd, struct socket_message& rstMessage) {
	SocketClientCtx* pstClientCtx = (SocketClientCtx*)rstServerCtx.m_stClientPool.GetClientByFd(iClientFd);
	if(NULL == pstClientCtx)
	{
		LOGERROR("client<{}> recv data but not in client pool.", iClientFd);
		return;
	}
	if(NULL == rstMessage.data || rstMessage.ud <= 0){
		LOGERROR("client<{}> recv data but buffer invalid.", iClientFd);
		return;
	}

	BYTE* pchBufferPtr = (BYTE*)rstMessage.data;
	int32_t iBufferNeedRead = (int32_t)rstMessage.ud;
	do{
		int32_t iReadLen = pstClientCtx->PacketBufferRead(pchBufferPtr, iBufferNeedRead);
		if(iReadLen > 0)
		{
			pchBufferPtr += iReadLen;
			iBufferNeedRead -= iReadLen;
		}
		int32_t iRet = rstServerCtx.ExecuteCmdRecvAllPacket(pstClientCtx);
		if(0 != iRet)
		{
			_ProcessCloseClient(pstServer, rstServerCtx, iClientFd);
			return;
		}
	}while(iBufferNeedRead > 0);

}

/*
	这个函数的线程只能转发消息出去，不能调用任何的可能触发 send_quest 函数的逻辑
*/
inline void _ProcessOneLoop(struct socket_server* pstServer, ServerContext& rstServerCtx)
{
	struct socket_message stMessage;
	int type = socket_server_poll(pstServer, &stMessage, NULL);

	int32_t iClientFd = 0;
	switch (type)
	{
	case SOCKET_EXIT:
		LOGINFO("socket server exited.");
		return;
	case SOCKET_OPEN:
		LOGINFO("socket server open.");
		break;
	case SOCKET_ACCEPT:
		iClientFd = stMessage.ud;
		_ProcessNewClient(pstServer, rstServerCtx, iClientFd, stMessage);
		socket_server_start(pstServer, NETWORK_SOCKET_SERVER_OPAQUE, iClientFd);
		break;
	case SOCKET_CLOSE:
		iClientFd = stMessage.id;
		_ProcessCloseClient(pstServer, rstServerCtx, iClientFd);
		break;
	case SOCKET_ERROR:
		iClientFd = stMessage.id;
		_ProcessCloseClient(pstServer, rstServerCtx, iClientFd);
		break;
	case SOCKET_DATA:
		iClientFd = stMessage.id;
		_ProcessMessageData(pstServer, rstServerCtx, iClientFd, stMessage);
		break;
	case SOCKET_UDP:
		break;
	default:
		LOGINFO("unknown socket message type {}.", type);
		break;
	}
	return;
}

void SocketServer::Init(struct ST_ServerIPInfo& rstServerIPInfo, ServerContext& rstServerCtx)
{
    if(false == CreatePipe(m_iRecvCtrlFd, m_iSendCtrlFd))
    {
        LOGCRITICAL("create pipe failed!.");
        return;
    }

	m_pstSocketServer = socket_server_create();
	assert(m_pstSocketServer);

	m_pstServerCtx = &rstServerCtx;

    int l = socket_server_listen(m_pstSocketServer, NETWORK_SOCKET_SERVER_OPAQUE, rstServerIPInfo.m_achServerIP, rstServerIPInfo.m_wServerPort, NETWORK_SERVER_LISTEN_QUEUE_LEN);
    socket_server_start(m_pstSocketServer, NETWORK_SOCKET_SERVER_OPAQUE, l);
}

void SocketServer::SendNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
{
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
        if(n != sizeof(rstNetCmd))
        {
            LOGFATAL("SendNetworkCmd write cmd error, success write: %ld cmd size: %ld", n, sizeof(rstNetCmd));
        }
        return;
    }
}

void SocketServer::RecvNetworkCmd(struct ST_NETWORK_CMD_REQUEST& rstNetCmd)
{
	assert(m_pstSocketServer);
	assert(m_pstServerCtx);

	BYTE bchNetCmd = rstNetCmd.GetCmd();
	switch(bchNetCmd)
	{
		case NETWORK_CMD_REQUEST_SEND_ALL_PACKET:
			LOGINFO("execute cmd send all");
            m_pstServerCtx->ExecuteCmdSendAllPacket(rstNetCmd);
			break;
		case NETWORK_CMD_REQUEST_CLOSE_SERVER:
			LOGINFO("execute cmd close all");
			socket_server_exit(m_pstSocketServer);
			socket_server_release(m_pstSocketServer);
			break;
		default:
			LOGERROR("unknown net cmd: {}", (char)bchNetCmd);
			break;
	}
}

static void* StartThreadRecvLoop(void* pVoidServer)
{
	LOGINFO("ThreadRecvLoop start.");

	SocketServer* pstSocketServer = (SocketServer*)pVoidServer;
	assert(pstSocketServer);
	assert(pstSocketServer->m_pstSocketServer);
	assert(pstSocketServer->m_pstServerCtx);
	for(;;)
	{
		_ProcessOneLoop(pstSocketServer->m_pstSocketServer, *(pstSocketServer->m_pstServerCtx));
	}

	LOGINFO("ThreadRecvLoop finished.");
}

static void* StartThreadCmdLoop(void* pVoidServer)
{
	LOGINFO("ThreadCmdLoop start.");

	SocketServer* pstSocketServer = (SocketServer*)pVoidServer;
	assert(pstSocketServer);
	assert(pstSocketServer->m_pstSocketServer);
	assert(pstSocketServer->m_pstServerCtx);

	int32_t iCmdSize = (int32_t)sizeof(ST_NETWORK_CMD_REQUEST);
	BYTE chBuffer[iCmdSize];
	memset(chBuffer, 0, sizeof(chBuffer));

	for(;;)
	{
		int32_t n = read(pstSocketServer->m_iRecvCtrlFd, chBuffer, iCmdSize);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			LOGFATAL("read pipe error: {}.", strerror(errno));
			return NULL;
		}

		ST_NETWORK_CMD_REQUEST* pstNetCmd = (ST_NETWORK_CMD_REQUEST*)chBuffer;
		pstSocketServer->RecvNetworkCmd(*pstNetCmd);
	}

	LOGINFO("ThreadCmdLoop finished.");
	return NULL;
}

void SocketServer::StartEventLoop()
{
	assert(m_pstSocketServer);
	assert(m_pstServerCtx);

    pthread_create(&g_SocketThreadPool[0], NULL, StartThreadRecvLoop, this);
    pthread_create(&g_SocketThreadPool[1], NULL, StartThreadCmdLoop, this);
    if(g_SocketThreadPool[0] != 0){
        pthread_join(g_SocketThreadPool[0], NULL);
        LOGINFO("线程 StartThreadRecvLoop 已结束！");
    }
    if(g_SocketThreadPool[1] != 0){
        pthread_join(g_SocketThreadPool[1], NULL);
        LOGINFO("线程 StartThreadCmdLoop 已结束！");
    }

}

SocketServer::SocketServer()
{
	construct();
}

SocketServer::~SocketServer()
{
	construct();
}

void SocketServer::construct()
{
	m_pstSocketServer = NULL;
	m_pstServerCtx = NULL;
	m_iRecvCtrlFd = 0;
	m_iSendCtrlFd = 0;
}

} // namespace network

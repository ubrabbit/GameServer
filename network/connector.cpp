
#include "connector.h"

namespace network {

void _OnConnectorBufferEventWrite(struct bufferevent* pstEvtobj, void* pstVoidConnector)
{
	LOGINFO("_OnConnectorBufferEventWrites");
}

void _OnConnectorBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidConnector)
{
	LOGINFO("_OnConnectorBufferEventRead");

	assert(pstVoidConnector);
	Connector* pstConnector = (Connector*)pstVoidConnector;
	Connector& rstConnector = *pstConnector;

	ConnectorContext* pstConnectorCtx = rstConnector.ConnectorCtx();
	if(NULL == pstConnectorCtx)
	{
		LOGFATAL("{} read error by connector_ctx not found!", rstConnector.Repr());
		return;
	}

	LibeventClientCtx* pstClientCtx = rstConnector.ClientCtx();
	if(NULL == pstClientCtx)
	{
		LOGFATAL("{} read error by client_ctx not found!", rstConnector.Repr());
		return;
	}

	int32_t iReadLen = 0;
	do{
		iReadLen = pstClientCtx->PacketBufferRead(pstEvtobj);
		int32_t iRet = pstConnectorCtx->ExecuteCmdRecvAllPacket(pstClientCtx);
		if(0 != iRet)
		{
			LOGFATAL("{} read error: {}", rstConnector.Repr(), iRet);
			return;
		}
		//第一次read缓冲区可能满了，正常情况下会解包空出空间，所以再收一次
		iReadLen = pstClientCtx->PacketBufferRead(pstEvtobj);
	}while(iReadLen > 0);
}

void _OnConnectorBufferEventTrigger(struct bufferevent* pstEvtobj, short wEvent, void* pstVoidConnector)
{
	LOGINFO("_OnConnectorBufferEventTrigger");

	assert(pstVoidConnector);
	Connector* pstConnector = (Connector*)pstVoidConnector;
	Connector& rstConnector = *pstConnector;

	if(wEvent & BEV_EVENT_TIMEOUT)
	{
		LOGFATAL("{} timeout", rstConnector.Repr());
		rstConnector.SetDead();
	}
	if(wEvent & BEV_EVENT_EOF)
	{
		LOGFATAL("{} read EOF error", rstConnector.Repr());
		rstConnector.SetDead();
	}
	if(wEvent & BEV_EVENT_ERROR)
	{
		int32_t iErrCode = evutil_socket_geterror(bufferevent_getfd(pstEvtobj));
		LOGFATAL("{} read error: {}", rstConnector.Repr(), evutil_socket_error_to_string(iErrCode));
		rstConnector.SetDead();
	}

}

bool Connector::Create()
{
    evthread_use_pthreads();
	m_pstEventBase = event_base_new();

	m_iSockFd = socket(PF_INET, SOCK_STREAM, 0);
	if(m_iSockFd < 0)
	{
		LOGFATAL("can't create socket: {}", strerror(errno));
		return false;
	}

	struct bufferevent* pstBufferEvt = bufferevent_socket_new(
										m_pstEventBase, m_iSockFd,
										BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
	if(NULL == pstBufferEvt)
	{
		LOGFATAL("bufferevent_socket_new error: {}", strerror(errno));
		return false;
	}

	m_pstBufferEvt = pstBufferEvt;
	return true;
}

bool Connector::Connect()
{
	assert(m_pstEventBase);
	assert(m_pstBufferEvt);

	struct sockaddr_in fsin;
	memset(&fsin, 0, sizeof(fsin));
	fsin.sin_family = AF_INET;
	fsin.sin_addr.s_addr = inet_addr(m_stServerIPInfo.m_achServerIP);
	fsin.sin_port = htons((unsigned short)m_stServerIPInfo.m_wServerPort);

	bufferevent_setcb(m_pstBufferEvt,
					_OnConnectorBufferEventRead,
					_OnConnectorBufferEventWrite,
					_OnConnectorBufferEventTrigger,
					this);
	bufferevent_enable(m_pstBufferEvt, EV_READ|EV_WRITE|EV_TIMEOUT|EV_PERSIST);

	struct timeval tv = {60, 0};
	bufferevent_set_timeouts(m_pstBufferEvt, NULL, &tv);
	if (bufferevent_socket_connect(m_pstBufferEvt, (struct sockaddr *)&fsin, sizeof(fsin)) < 0)
	{
		LOGFATAL("bufferevent_socket_connect error: {}", strerror(errno));
		return false;
	}

	m_pstClientCtx = new LibeventClientCtx(m_iSockFd, m_pstBufferEvt, fsin);
	m_pstConnectorCtx = new ConnectorContext(m_pstClientCtx);
	return true;
}

void Connector::Close()
{
	m_bIsAlive = false;
	if(NULL != m_pstBufferEvt)
	{
		bufferevent_free(m_pstBufferEvt);
		m_pstBufferEvt = NULL;
	}
	if(NULL != m_pstEventBase)
	{
		event_base_free(m_pstEventBase);
		m_pstEventBase = NULL;
	}
	if(NULL != m_pstClientCtx)
	{
		delete m_pstClientCtx;
		m_pstClientCtx = NULL;
	}
	if(NULL != m_pstConnectorCtx)
	{
		delete m_pstConnectorCtx;
		m_pstConnectorCtx = NULL;
	}
}

void Connector::SetAlive()
{
	m_bIsAlive = true;
}

void Connector::SetDead()
{
	m_bIsAlive = false;
}

bool Connector::IsAlive()
{
	return m_bIsAlive;
}

int32_t Connector::SocketFd()
{
	return m_iSockFd;
}

LibeventClientCtx* Connector::ClientCtx()
{
	assert(m_pstClientCtx);
	return m_pstClientCtx;
}

ConnectorContext* Connector::ConnectorCtx()
{
	assert(m_pstConnectorCtx);
	return m_pstConnectorCtx;
}

std::string Connector::Repr()
{
	char chBuffer[128] = {0};
	sprintf(chBuffer, "Connector<%s:%d>", m_stServerIPInfo.m_achServerIP, m_stServerIPInfo.m_wServerPort);
	return std::string(chBuffer);
}

void* ConnectorMainThread(void* conn)
{
	assert(conn);
	Connector* pstConnector = (Connector*)conn;
	Connector& rstConnector = *pstConnector;

	LOGINFO("{} main thread start.", rstConnector.Repr());
	if(false == rstConnector.Connect())
	{
		LOGFATAL("{} connect error: {}", rstConnector.Repr(), strerror(errno));
		rstConnector.SetDead();
	}
	else
	{
		LOGFATAL("{} connect success.", rstConnector.Repr());
		rstConnector.SetAlive();
	}

	while(rstConnector.IsAlive())
	{
		rstConnector.ProcessSendNetworkPackets();
		SleepMicroSeconds(10);
	}
	LOGINFO("{} main thread finished.", rstConnector.Repr());

	rstConnector.Close();
	CloseConnector(rstConnector.SocketFd());

	delete pstConnector;
	return NULL;
}

void Connector::StartMainLoop()
{
    if(false == CreatePipe(m_iRecvCtrlFd, m_iSendCtrlFd))
    {
        LOGCRITICAL("{} create pipe failed!.", Repr());
        return;
    }
	LOGINFO("{} start connect thread.", Repr());
    pthread_create(&m_stMainThread, NULL, ConnectorMainThread, this);
}

void Connector::SendNetworkPackets()
{
	if(false == IsAlive())
	{
		return;
	}

	assert(m_pstConnectorCtx);
	// 发送命令给connector主线程，让connector主线程完成发送，避免不同线程对connector对象操作进行加锁。
	struct ST_NETWORK_CMD_REQUEST stNetCmd(NETWORK_CMD_REQUEST_SEND_ALL_PACKET);
    for(;;)
    {
        const char* pchRequest = (const char*)&stNetCmd;
        ssize_t n = write(m_iSendCtrlFd, pchRequest, sizeof(stNetCmd));
        if(n<0) {
            if (errno != EINTR) {
                LOGERROR("{} send ctrl command {} error: {}", Repr(), stNetCmd.m_Header, strerror(errno));
            }
            continue;
        }
        if(n != sizeof(stNetCmd))
        {
            LOGFATAL("{} SendNetworkCmd write cmd error, success write: %ld cmd size: %ld", Repr(), n, sizeof(stNetCmd));
        }
        return;
    }
}

void Connector::ProcessSendNetworkPackets()
{
	assert(m_pstConnectorCtx);
	if(false == IsAlive())
	{
		LOGINFO("{} skip send packets by not alived.", Repr());
		return;
	}

	int32_t iCmdSize = (int32_t)sizeof(ST_NETWORK_CMD_REQUEST);
	BYTE chBuffer[iCmdSize];
	memset(chBuffer, 0, sizeof(chBuffer));

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100 * 1000;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(m_iRecvCtrlFd, &set);

	for(;;)
	{
		int32_t n = 0;
		int32_t ret = select(m_iRecvCtrlFd, &set, NULL, NULL, &timeout);
		if(ret == -1) // error
		{
			LOGFATAL("{} select fd<{}> error: {}", Repr(), m_iRecvCtrlFd, strerror(errno));
			SetDead();
			break;
		}
		else if(ret == 0) // timeout
		{
			break;
		}

		n = read(m_iRecvCtrlFd, chBuffer, iCmdSize);
		if (n <= 0) {
			if (errno == EINTR)
				continue;;
			LOGFATAL("{} read pipe error: {}.", Repr(), strerror(errno));
			SetDead();
			break;
		}
		if(false == IsAlive())
		{
			LOGINFO("{} skip send packets by not alived.", Repr());
			break;
		}

		ST_NETWORK_CMD_REQUEST* pstNetCmd = (ST_NETWORK_CMD_REQUEST*)chBuffer;
		ST_NETWORK_CMD_REQUEST& rstNetCmd = *pstNetCmd;
		BYTE bchNetCmd = rstNetCmd.GetCmd();
		switch(bchNetCmd)
		{
			case NETWORK_CMD_REQUEST_SEND_ALL_PACKET:
				LOGINFO("{} sexecute cmd send all", Repr());
				m_pstConnectorCtx->ExecuteCmdSendAllPacket(rstNetCmd);
				break;
			case NETWORK_CMD_REQUEST_CLOSE_SERVER:
				LOGINFO("{} sexecute cmd close", Repr());
				SetDead();
				break;
			default:
				LOGERROR("{} sunknown net cmd: {}", Repr(), (char)bchNetCmd);
				break;
		}
	}
}

bool ConnectorPool::AddConnector(Connector* pstConnector)
{
	int32_t iSockFd = pstConnector->SocketFd();
	auto it = m_stConnectorPool.find(iSockFd);
	if(it != m_stConnectorPool.end())
	{
		LOGFATAL("AddConnector iSockFd<{}> fail by already exists in pool!", iSockFd);
		return false;
	}
	m_stConnectorPool.insert(std::pair<int32_t, Connector*>(iSockFd, pstConnector));
	return true;
}

bool ConnectorPool::CloseConnector(int32_t iSockFd)
{
	auto it = m_stConnectorPool.find(iSockFd);
	if(it != m_stConnectorPool.end())
	{
		Connector* pstConnector = it->second;
		pstConnector->SetDead();	// 不主动关闭，让connector的主线程结束后再关
		m_stConnectorPool.erase(it);
		return true;
	}
	return false;
}

void ConnectorPool::ProcessAllConnector(std::vector<Connector*>& rvecContexQueue)
{
    for(auto it=m_stConnectorPool.begin(); it!=m_stConnectorPool.end(); it++)
    {
		Connector* pstConnector = it->second;
		if(false == pstConnector->IsAlive())
		{
			continue;
		}
		rvecContexQueue.push_back(pstConnector);
    }
}

Connector* NewConnector(struct ST_ServerIPInfo& rstServerIPInfo)
{
	Connector* pstConnector = new Connector(rstServerIPInfo);
	if(false == pstConnector->Create())
	{
		LOGFATAL("create connector failed");
		pstConnector->Close();
		delete pstConnector;
		return NULL;
	}

	ConnectorPool::Instance().AddConnector(pstConnector);
	return pstConnector;
}

bool CloseConnector(int32_t iSockFd)
{
	return ConnectorPool::Instance().CloseConnector(iSockFd);
}

void ProcessAllConnector(std::vector<Connector*>& rvecContexQueue)
{
	return ConnectorPool::Instance().ProcessAllConnector(rvecContexQueue);
}

} // namespace network

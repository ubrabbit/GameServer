
#include "connector.h"

namespace network {

void _OnConnectorBufferEventWrite(int32_t iSockfd, short wEvent, void* pstVoidConnector)
{
	assert(pstVoidConnector);
	Connector* pstConnector = (Connector*)pstVoidConnector;
	Connector& rstConnector = *pstConnector;

	rstConnector.ProcessSendNetworkPackets();
}

void _OnConnectorBufferEventRead(struct bufferevent* pstEvtobj, void* pstVoidConnector)
{
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
	m_ulClientSeq = SnowFlakeUUID::Instance().GenerateSeqID();

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
					NULL,
					_OnConnectorBufferEventTrigger,
					this);
	bufferevent_enable(m_pstBufferEvt, EV_READ|EV_WRITE|EV_TIMEOUT|EV_PERSIST);
	m_pstWriteEvent = event_new(m_pstEventBase, -1, EV_WRITE, _OnConnectorBufferEventWrite, this);

	struct timeval tv = {60, 0};
	bufferevent_set_timeouts(m_pstBufferEvt, NULL, &tv);
	if (bufferevent_socket_connect(m_pstBufferEvt, (struct sockaddr *)&fsin, sizeof(fsin)) < 0)
	{
		LOGFATAL("bufferevent_socket_connect error: {}", strerror(errno));
		return false;
	}

	m_pstClientCtx = new LibeventClientCtx(m_iSockFd, m_pstBufferEvt, fsin);
	m_pstConnectorCtx = new ConnectorContext(m_ulClientSeq, m_pstClientCtx);
	return true;
}

void Connector::Close()
{
	SetClosed();

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
	if(NULL != m_pstWriteEvent)
	{
		event_free(m_pstWriteEvent);
		m_pstWriteEvent = NULL;
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
	m_iStatus = NEWTOWK_CONNECTOR_STATUS_ALIVE;
}

void Connector::SetDead()
{
	m_iStatus = NEWTOWK_CONNECTOR_STATUS_DEAD;
}

void Connector::SetClosed()
{
	m_iStatus = NEWTOWK_CONNECTOR_STATUS_CLOSED;
}

bool Connector::IsAlive()
{
	return m_iStatus == NEWTOWK_CONNECTOR_STATUS_ALIVE;
}

bool Connector::IsDead()
{
	return m_iStatus == NEWTOWK_CONNECTOR_STATUS_DEAD;
}

bool Connector::IsClosed()
{
	return m_iStatus == NEWTOWK_CONNECTOR_STATUS_CLOSED;
}

int32_t Connector::SocketFd()
{
	return m_iSockFd;
}

uint64_t Connector::ClientSeq()
{
	return m_ulClientSeq;
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
	sprintf(chBuffer, "Connector<%ld><%d><%s:%d>", m_ulClientSeq, m_iSockFd, m_stServerIPInfo.m_achServerIP, m_stServerIPInfo.m_wServerPort);
	return std::string(chBuffer);
}

void* ConnectorMainThread(void* conn)
{
	assert(conn);
	Connector* pstConnector = (Connector*)conn;
	Connector& rstConnector = *pstConnector;

	LOGINFO("{} main thread start.", rstConnector.Repr());
	for(int32_t i=0; i<30; i++)
	{
		if(false == rstConnector.Connect())
		{
			LOGFATAL("{} connect error: {}", rstConnector.Repr(), strerror(errno));
			rstConnector.SetDead();
			SleepMilliSeconds(1000);
			continue;
		}
		else
		{
			LOGFATAL("{} connect success.", rstConnector.Repr());
			rstConnector.SetAlive();
			break;
		}
	}

	rstConnector.StartMainLoop();
	LOGINFO("{} main thread finished.", rstConnector.Repr());
	rstConnector.Close();
	rstConnector.SetClosed();
	return NULL;
}

void Connector::StartMainThread()
{
    if(false == CreatePipe(m_iRecvCtrlFd, m_iSendCtrlFd))
    {
        LOGCRITICAL("{} create pipe failed!.", Repr());
        return;
    }
	LOGINFO("{} start connect thread.", Repr());
    pthread_create(&m_stMainThread, NULL, ConnectorMainThread, this);
}

void Connector::StartMainLoop()
{
	assert(m_pstEventBase);
	while(IsAlive())
	{
		HeartBeat();
		event_base_loop(m_pstEventBase, EVLOOP_NONBLOCK | EVLOOP_ONCE);
		SleepMicroSeconds(100);
	}
}

void Connector::SendNetworkPackets(ConnectorContext& Ctx)
{
	if(false == IsAlive())
	{
		return;
	}

	Ctx.PacketSendPrepare();

	// 发送命令给connector主线程，让connector主线程完成发送，避免不同线程对connector对象操作进行加锁。
	struct ST_NETWORK_CMD_REQUEST stNetCmd(NETWORK_CMD_REQUEST_SEND_ALL_PACKET);
    for(;;)
    {
		// 需要在循环开头再判断一次Alive
		if(false == IsAlive())
		{
			break;
		}
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
		event_active(m_pstWriteEvent, EV_WRITE, 0);
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
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(m_iRecvCtrlFd, &set);

	for(;;)
	{
		int32_t ret = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
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

		int32_t n = read(m_iRecvCtrlFd, chBuffer, iCmdSize);
		if (n <= 0) {
			if (errno == EINTR)
			{
				continue;;
			}
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
				//LOGINFO("{} execute cmd send all", Repr());
				m_pstConnectorCtx->ExecuteCmdSendAllPacket(rstNetCmd);
				break;
			case NETWORK_CMD_REQUEST_CLOSE_SERVER:
				LOGINFO("{} execute cmd close", Repr());
				SetDead();
				break;
			default:
				LOGERROR("{} unknown net cmd: {}", Repr(), (char)bchNetCmd);
				break;
		}
	}
}

void Connector::HeartBeat()
{
	m_iHeartBeatCnt++;
	if(m_iHeartBeatCnt % 10000 == 0)
	{
		LOGINFO("{} heartbeat.", Repr());
		m_iHeartBeatCnt = 0;

		ConnectorContext& rstConnectorContext = *m_pstConnectorCtx;
		ProtoHello::SSHello stHello;
		stHello.set_timestamp(GetMilliSecond());
		rstConnectorContext.PacketProduceProtobufPacket(ClientSeq(), SocketFd(), SS_PROTOCOL_MESSAGE_ID_HELLO, stHello);
		SendNetworkPackets(rstConnectorContext);
	}
}

bool ConnectorPool::AddConnector(Connector* pstConnector)
{
	uint64_t iClientSeq = pstConnector->ClientSeq();
	auto it = m_stConnectorPool.find(iClientSeq);
	if(it != m_stConnectorPool.end())
	{
		LOGFATAL("AddConnector {} fail by already exists in pool!", pstConnector->Repr());
		return false;
	}
	m_stConnectorPool.insert(std::pair<int32_t, Connector*>(iClientSeq, pstConnector));
	return true;
}

bool ConnectorPool::CloseConnector(uint64_t iClientSeq)
{
	auto it = m_stConnectorPool.find(iClientSeq);
	if(it != m_stConnectorPool.end())
	{
		Connector* pstConnector = it->second;
		if(pstConnector->IsAlive())
		{
			LOGFATAL("{} close but still alive!", pstConnector->Repr());
		}
		m_stConnectorPool.erase(it);
		delete pstConnector;
		return true;
	}
	return false;
}

void ConnectorPool::ProcessAliveConnectors(std::vector<Connector*>& rvecContexQueue)
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

void ConnectorPool::ProcessClosedConnectors()
{
	auto it=m_stConnectorPool.begin();
	while(it != m_stConnectorPool.end())
    {
		Connector* pstConnector = it->second;
		if(pstConnector->IsClosed())
		{
			m_stConnectorPool.erase(it);
			delete pstConnector;
		}
		else
		{
			++it;
		}
    }
}

Connector* NewConnector(struct ST_ServerIPInfo& rstServerIPInfo)
{
	Connector* pstConnector = new Connector(rstServerIPInfo);
	if(false == pstConnector->Create())
	{
		LOGFATAL("create connector failed");
		delete pstConnector;
		return NULL;
	}

	ConnectorPool::Instance().AddConnector(pstConnector);
	return pstConnector;
}

void ProcessAliveConnectors(std::vector<Connector*>& rvecContexQueue)
{
	return ConnectorPool::Instance().ProcessAliveConnectors(rvecContexQueue);
}

void ProcessClosedConnectors()
{
	return ConnectorPool::Instance().ProcessClosedConnectors();
}

} // namespace network

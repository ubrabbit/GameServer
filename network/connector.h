#pragma once

#include <map>
#include <pthread.h>
#include <errno.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/thread.h>

#include "common/singleton.h"
#include "common/utils/spinlock.h"
#include "common/utils/time.h"

#include "libevent/libevent_ctx.h"

#include "defines.h"
#include "queue.h"
#include "context.h"

namespace network {

class Connector
{
public:

    Connector(struct ST_ServerIPInfo stServerIPInfo):
        m_stServerIPInfo(stServerIPInfo),
        m_pstEventBase(NULL),
        m_pstBufferEvt(NULL),
        m_pstClientCtx(NULL),
        m_pstConnectorCtx(NULL),
        m_iSockFd(0),
        m_bIsAlive(false)
    {}

    ~Connector()
    {
        Close();
    }

    std::string Repr();

    bool Create();
    bool Connect();
    void Close();

    void SetAlive();
    void SetDead();
    bool IsAlive();

    int32_t SocketFd();
    LibeventClientCtx* ClientCtx();
    ConnectorContext*  ConnectorCtx();

    void StartMainLoop();
    void SendNetworkPackets();
    void ProcessSendNetworkPackets();

private:
    struct ST_ServerIPInfo  m_stServerIPInfo;
    struct event_base*      m_pstEventBase;
    struct bufferevent*     m_pstBufferEvt;
    LibeventClientCtx*      m_pstClientCtx;
    ConnectorContext*       m_pstConnectorCtx;

	struct spinlock		    m_stReadLock;
	struct spinlock		    m_stWriteLock;
    int32_t                 m_iSockFd;

    bool                    m_bIsAlive;

    pthread_t               m_stMainThread;

    int32_t                 m_iRecvCtrlFd;
    int32_t                 m_iSendCtrlFd;

};

class ConnectorPool : public Singleton<ConnectorPool>
{
public:

    bool AddConnector(Connector* pstConnector);
    bool CloseConnector(int32_t iSockFd);
    void ProcessAllConnector(std::vector<Connector*>& rvecContexQueue);

private:
    std::map<int32_t, Connector*>   m_stConnectorPool;

};

Connector* NewConnector(struct ST_ServerIPInfo& rstServerIPInfo);
bool CloseConnector(int32_t iSockFd);
void ProcessAllConnector(std::vector<Connector*>& rvecContexQueue);

} // namespace network

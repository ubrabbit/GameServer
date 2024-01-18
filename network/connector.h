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

#include "proto/proto.h"
#include "proto/distcpp/hello.pb.h"

#include "defines.h"
#include "queue.h"
#include "context.h"

namespace network {

typedef enum NEWTOWK_CONNECTOR_STATUS_TYPE
{
    NEWTOWK_CONNECTOR_STATUS_UNCONNECT  = 0,
    NEWTOWK_CONNECTOR_STATUS_ALIVE      = 1,
    NEWTOWK_CONNECTOR_STATUS_DEAD       = 2,
    NEWTOWK_CONNECTOR_STATUS_CLOSED     = 3
}NEWTOWK_CONNECTOR_STATUS_TYPE;

class Connector
{
public:

    Connector(struct ST_ServerIPInfo stServerIPInfo):
        m_stServerIPInfo(stServerIPInfo),
        m_pstEventBase(NULL),
        m_pstBufferEvt(NULL),
        m_pstWriteEvent(NULL),
        m_pstClientCtx(NULL),
        m_pstConnectorCtx(NULL),
        m_ulClientSeq(0),
        m_iSockFd(0),
        m_iStatus(NEWTOWK_CONNECTOR_STATUS_UNCONNECT),
        m_iHeartBeatCnt(0)
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
    void SetClosed();
    bool IsAlive();
    bool IsDead();
    bool IsClosed();

    int32_t SocketFd();
    uint64_t ClientSeq();
    LibeventClientCtx* ClientCtx();
    ConnectorContext*  ConnectorCtx();

    void StartMainThread();
    void StartMainLoop();

    void SendNetworkPackets(ConnectorContext& Ctx);

    void ProcessSendNetworkPackets();

    void HeartBeat();

private:
    struct ST_ServerIPInfo  m_stServerIPInfo;
    struct event_base*      m_pstEventBase;
    struct bufferevent*     m_pstBufferEvt;
    struct event*           m_pstWriteEvent;

    LibeventClientCtx*      m_pstClientCtx;
    ConnectorContext*       m_pstConnectorCtx;

	struct spinlock		    m_stReadLock;
	struct spinlock		    m_stWriteLock;

    uint64_t                m_ulClientSeq;
    int32_t                 m_iSockFd;

    int32_t                 m_iStatus;

    pthread_t               m_stMainThread;

    int32_t                 m_iRecvCtrlFd;
    int32_t                 m_iSendCtrlFd;

    int32_t                 m_iHeartBeatCnt;

};

class ConnectorPool : public Singleton<ConnectorPool>
{
public:

    bool AddConnector(Connector* pstConnector);
    bool CloseConnector(uint64_t iClientSeq);
    void ProcessAliveConnectors(std::vector<Connector*>& rvecContexQueue);
    void ProcessClosedConnectors();

private:
    std::map<uint64_t, Connector*>   m_stConnectorPool;

};

Connector* NewConnector(struct ST_ServerIPInfo& rstServerIPInfo);
bool CloseConnector(uint64_t iClientSeq);
void ProcessAliveConnectors(std::vector<Connector*>& rvecContexQueue);
void ProcessClosedConnectors();

} // namespace network

#include "gamelogic.h"

using namespace network;
using namespace gametimer;
using namespace gameevent;
using namespace gamebinding;


void GameLogic::_ProcessTimerCallback(Gamesvr& rstGamesvr)
{
    m_stTimerWaitQueue.PrepareExecute(m_stTimerExecuteQueue);
    for(auto it=m_stTimerExecuteQueue.m_vecQueue.begin(); it!=m_stTimerExecuteQueue.m_vecQueue.end(); it++)
    {
        ST_TimerContext& ctx = *it;
        ctx.Callback();
    }
    m_stTimerExecuteQueue.Init();
}

void GameLogic::_ProcessEventCallback(Gamesvr& rstGamesvr)
{
    GameEventListener::Instance().ProcessOnceLoop();
}

template<typename Owner, typename Context>
void GameLogic::_ProcessSSNetworkPacketHandle(Owner& O, Context& Ctx, int32_t iPacketNum, NetPacketQueue& rstPacketQueue)
{
    assert(iPacketNum == rstPacketQueue.Size());

    // 用下标出错直接coredump，用迭代器可能会写出惊天奇BUG
    for(int32_t i=0; i<iPacketNum; i++)
    {
        NetPacketBuffer& rstPacket = rstPacketQueue.m_vecPacketPool[i];
        gamehandle::HandleSSProto(Ctx, rstPacket);
    }
    O.SendNetworkPackets();
}

void GameLogic::_ProcessSSNetworkPacket(Gamesvr& rstGamesvr)
{
    size_t dwStartTime = GetMilliSecond();
    int32_t iTotalNum = 0;

    // proxy 收到的包
    Server& rstServer = rstGamesvr.m_stProxyServer;
    ServerContext& rstServerCtx = rstServer.m_stServerCtx;
    if(rstServerCtx.m_stSignalExec.CheckAndSetSignalActived())
    {
        int32_t iPacketNum = rstServerCtx.CopyPacketsFromNetToLogic();
        _ProcessSSNetworkPacketHandle(rstServer, rstServerCtx, iPacketNum, rstServerCtx.m_stNetPacketLogicQueue);
        iTotalNum += iPacketNum;
    }

    // connector 收到的回包
    std::vector<Connector*> vecConnectorQueue;
    ProcessAliveConnectors(vecConnectorQueue);
    for(auto it=vecConnectorQueue.begin(); it!=vecConnectorQueue.end(); it++)
    {
		Connector* pstConnector = *it;
		if(false == pstConnector->IsAlive())
		{
			continue;
		}
		ConnectorContext* pstConnectorCtx = pstConnector->ConnectorCtx();
		if(NULL == pstConnectorCtx)
		{
			LOGFATAL("{} process net packet but connector ctx is NULL!", pstConnector->Repr());
			continue;
		}
        if(false == pstConnectorCtx->m_stSignalExec.CheckAndSetSignalActived())
        {
            continue;
        }

        Connector& rstConnector = *pstConnector;
        ConnectorContext& rstConnectorCtx = *pstConnectorCtx;
        int32_t iPacketNum = rstConnectorCtx.CopyPacketsFromNetToLogic();
        _ProcessSSNetworkPacketHandle(rstConnector, rstConnectorCtx, iPacketNum, rstConnectorCtx.m_stNetPacketLogicQueue);
        iTotalNum += iPacketNum;
    }
    ProcessClosedConnectors();

    size_t dwCostTime = GetMilliSecond() - dwStartTime;
    if(iTotalNum > 0 && dwCostTime > 10)
    {
        LOGINFO("_ProcessSSNetworkPacket Finished: TotalPackets: {} Cost: {} ms", iTotalNum, dwCostTime);
    }
}

void GameLogic::_ProcessCSNetworkPacketHandle(Server& rstServer, ServerContext& rstServerCtx, int32_t iPacketNum, NetPacketQueue& rstPacketQueue)
{
    assert(iPacketNum == rstPacketQueue.Size());

    // 用下标出错直接coredump，用迭代器可能会写出惊天奇BUG
    for(int32_t i=0; i<iPacketNum; i++)
    {
        NetPacketBuffer& rstPacket = rstPacketQueue.m_vecPacketPool[i];
        gamehandle::HandleCSProto(rstServerCtx, rstPacket);

        if(i > 0 && i % 2000 == 0)
        {
            rstServer.SendNetworkPackets();
        }
    }
    rstServer.SendNetworkPackets();
}

int32_t GameLogic::_ProcessCSNetworkPacket(Gamesvr& rstGamesvr)
{
    Server& rstServer = rstGamesvr.m_stMainServer;
    ServerContext& rstServerCtx = rstServer.m_stServerCtx;
    if(false == rstServerCtx.m_stSignalExec.CheckAndSetSignalActived())
    {
        return 0;
    }

    size_t dwStartTime = GetMilliSecond();
    int32_t iPacketNum = rstServerCtx.CopyPacketsFromNetToLogic();
    _ProcessCSNetworkPacketHandle(rstServer, rstServerCtx, iPacketNum, rstServerCtx.m_stNetPacketLogicQueue);
    size_t dwCostTime = GetMilliSecond() - dwStartTime;
    if(iPacketNum > 0 && dwCostTime > 10)
    {
        LOGINFO("_ProcessNetworkPacket Finished: TotalPackets: {} Cost: {} ms", iPacketNum, dwCostTime);
    }
    return iPacketNum;
}

void GameLogic::_ProcessOneLoop(Gamesvr& rstGamesvr)
{
    //处理定时器回调
    _ProcessTimerCallback(rstGamesvr);
    //处理事件回调
    _ProcessEventCallback(rstGamesvr);
    //处理SS网络包回调
    _ProcessSSNetworkPacket(rstGamesvr);
    //处理CS网络包回调
    _ProcessCSNetworkPacket(rstGamesvr);
}

void GameLogic::StartProxyConnector(Gamesvr& rstGamesvr)
{
    GameSvrConf& rstGameSvrConf = rstGamesvr.m_stGameSvrConf;
    Connector* pstConnector = NewConnector(rstGameSvrConf.m_stProxyServerIPInfo);
    if(NULL == pstConnector)
    {
        LOGCRITICAL("connect to <{}> failed!", rstGameSvrConf.m_stProxyServerIPInfo.Repr());
        return;
    }
    pstConnector->StartMainThread();
}

void GameLogic::StartMainLoop(Gamesvr& rstGamesvr)
{
    gamelog::BufferLogKFiFo::Instance().BindMainThread();
    PythonBinding::Instance().InitPythonBinding();

    do{
        LOGINFO("waiting network inited.");
        SleepMilliSeconds(1000);
    }
    while(false == ServerMonitor::Instance().IsInited());
    StartProxyConnector(rstGamesvr); // 自己的服务启动后，才启动连接器

    LOGINFO("logic mainloop start.");
    while(true)
    {
        _ProcessOneLoop(rstGamesvr);
        SleepMicroSeconds(100);     // 100微秒 == 1/10 毫秒
    }
    LOGINFO("logic mainloop finished.");

    PythonBinding::Instance().QuitPythonBinding();
}

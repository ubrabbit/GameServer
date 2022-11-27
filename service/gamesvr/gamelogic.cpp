#include "gamelogic.h"

using namespace network;
using namespace gametimer;

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

void GameLogic::_ProcessCSNetworkPacket(Gamesvr& rstGamesvr)
{
    Server& rstServer = rstGamesvr.m_stMainServer;
    ServerContext& rstServerCtx = rstServer.m_stServerCtx;
    if(false == rstServerCtx.m_stSignalExec.CheckAndSetSignalActived())
    {
        return;
    }

    size_t dwStartTime = GetMilliSecond();
    int32_t iPacketNum = rstServerCtx.CopyPacketsFromNetToLogic();
    NetPacketQueue& rstPacketLogicQueue = rstServerCtx.m_stNetPacketLogicQueue;
    // 用下标出错直接coredump，用迭代器可能会写出惊天奇BUG
    for(int32_t i=0; i<iPacketNum; i++)
    {
        NetPacketBuffer& rstPacket = rstPacketLogicQueue.m_vecPacketPool[i];
        gamehandle::HandleCSProto(rstServer, rstPacket);

        if(i > 0 && i % 1000 == 0)
        {
            if(rstServerCtx.PacketSendPrepare() > 0)
            {
                rstServer.SendNetworkPackets();
            }
        }
    }
    if(rstServerCtx.PacketSendPrepare() > 0)
    {
        rstServer.SendNetworkPackets();
    }

    size_t dwCostTime = GetMilliSecond() - dwStartTime;
    if(iPacketNum > 0)
    {
        LOGINFO("_ProcessCSNetworkPacket Finished: TotalPackets: {} Cost: {} ms", iPacketNum, dwCostTime);
    }
}

void GameLogic::_ProcessOneLoop(Gamesvr& rstGamesvr)
{
    //先处理定时器回调
    _ProcessTimerCallback(rstGamesvr);
    //处理网络包回调
    _ProcessCSNetworkPacket(rstGamesvr);
}

void GameLogic::StartMainLoop(Gamesvr& rstGamesvr)
{
    do{
        LOGINFO("waiting network inited.");
        sleep(1);
    }
    while(false == ServerMonitor::Instance().IsInited());

    LOGINFO("logic mainloop start.");
    while(true)
    {
        _ProcessOneLoop(rstGamesvr);
        SleepMicroSeconds(100);     // 100微秒 == 1/10 毫秒
    }
    LOGINFO("logic mainloop finished.");
}

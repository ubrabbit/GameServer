#include "gamelogic.h"

using namespace network;

void GameLogic::_ProcessTimerCallback(Gamesvr& rstGamesvr)
{
}

void GameLogic::_HandleCSProtoHello(Server& rstServer, NetPacketBuffer& rstPacket)
{
    size_t dwLogicTime = GetMilliSecond();
    int32_t iSockFd = rstPacket.GetClientFd();
    ProtoHello::CSHello stHello;
    UnpackProtobufStruct(stHello, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
    //LOGINFO("recv client<{}> packet, len<{}> proto<{}> content<'{}'>", iSockFd, rstPacket.GetPacketSize(), rstPacket.GetProtoNo(), stHello.content());

    ProtoHello::CSHelloRsp stHelloRsp;
    stHelloRsp.set_id(stHello.id());
    stHelloRsp.set_content(stHello.content());
    stHelloRsp.set_unpack_timestamp(rstPacket.GetCreateTime());
    stHelloRsp.set_logic_timestamp(dwLogicTime);
    rstServer.m_stServerCtx.PacketSendProduceProtobufPacket(iSockFd, CS_PROTOCOL_MESSAGE_ID_HELLO, stHelloRsp);
}

void GameLogic::_HandleCSProtocol(Server& rstServer, NetPacketBuffer& rstPacket)
{
    bool bReturnNoNext = true;

    switch(rstPacket.GetProtoNo())
    {
        case CS_PROTOCOL_MESSAGE_ID_HELLO:
            _HandleCSProtoHello(rstServer, rstPacket);
            break;
        default:
            bReturnNoNext = false;
            break;
    }

    //todo: script interface
    if(false == bReturnNoNext)
    {
    }

    return;
}

void GameLogic::_ProcessCSNetworkPacket(Gamesvr& rstGamesvr)
{
    size_t dwStartTime = GetMilliSecond();

    Server& rstServer = rstGamesvr.m_stMainServer;
    ServerContext& rstServerCtx = rstServer.m_stServerCtx;

    int32_t iPacketNum = rstServerCtx.CopyPacketsFromNetToLogic();
    NetPacketQueue& rstPacketLogicQueue = rstServerCtx.m_stNetPacketLogicQueue;
    struct ST_NETWORK_CMD_REQUEST stNetCmd(NETWORK_CMD_REQUEST_SEND_ALL_PACKET);
    // 用下标出错直接coredump，用迭代器可能会写出惊天奇BUG
    for(int32_t i=0; i<iPacketNum; i++)
    {
        NetPacketBuffer& rstPacket = rstPacketLogicQueue.m_vecPacketPool[i];
        _HandleCSProtocol(rstServer, rstPacket);

        if(i > 0 && i % 1000 == 0)
        {
            if(rstServerCtx.PacketSendPrepare() > 0)
            {
                rstServer.SendNetworkCmd(stNetCmd);
            }
        }
    }
    if(rstServerCtx.PacketSendPrepare() > 0)
    {
        rstServer.SendNetworkCmd(stNetCmd);
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
        SleepMilliSeconds(1);
    }
    LOGINFO("logic mainloop finished.");
}

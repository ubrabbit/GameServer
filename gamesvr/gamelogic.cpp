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
    rstServer.m_stServerRecvCtx.PacketSendProduceProtobufPacket(iSockFd, CS_PROTOCOL_MESSAGE_ID_HELLO, stHelloRsp);
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
    ST_PacketBufferPool& rstLogicRecvBufferPool = rstServer.m_stLogicRecvBufferPool;

    int32_t iPacketNum = rstServer.m_stServerRecvCtx.PacketCopyFromNetToLogic(rstLogicRecvBufferPool);
    struct ST_NETWORK_CMD_REQUEST stNetCmd(NETWORK_CMD_REQUEST_SEND_ALL_PACKET);
    for(int32_t i=0; i<iPacketNum; i++)
    {
        NetPacketBuffer& rstPacket = rstLogicRecvBufferPool.m_stPacketBufferPool[i];
        _HandleCSProtocol(rstServer, rstPacket);
        rstServer.SendNetworkCmd(stNetCmd);
    }
    if(rstServer.m_stServerRecvCtx.HasPacketToSend())
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

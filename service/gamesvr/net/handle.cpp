#include "handle.h"

using namespace network;

namespace gamehandle
{

void HandleCSProtoHello(Server& rstServer, NetPacketBuffer& rstPacket)
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
    rstServer.m_stServerCtx.PacketProduceProtobufPacket(iSockFd, CS_PROTOCOL_MESSAGE_ID_HELLO, stHelloRsp);
}

void HandleSSProto(Server& rstServer, NetPacketBuffer& rstPacket)
{
}

void HandleSSProto(Connector& rstConnector, NetPacketBuffer& rstPacket)
{
}

void HandleCSProto(Server& rstServer, NetPacketBuffer& rstPacket)
{
    bool bReturnNoNext = true;

    switch(rstPacket.GetProtoNo())
    {
        case CS_PROTOCOL_MESSAGE_ID_HELLO:
            HandleCSProtoHello(rstServer, rstPacket);
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

} // namespace

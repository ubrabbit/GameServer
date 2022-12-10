#include "handle.h"

using namespace network;

namespace gamehandle
{

void HandleCSProtoHello(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket)
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
    rstServerCtx.PacketProduceProtobufPacket(iSockFd, CS_PROTOCOL_MESSAGE_ID_HELLO, stHelloRsp);
}

template<typename Context>
void HandleSSProtoHello(Context& ctx, NetPacketBuffer& rstPacket)
{
    int32_t iSockFd = rstPacket.GetClientFd();
    ProtoHello::SSHello stHello;
    UnpackProtobufStruct(stHello, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
    LOGINFO("recv pair<{}> hello", iSockFd);

    ProtoHello::SSHelloRsp stHelloRsp;
    stHelloRsp.set_timestamp(GetMilliSecond());
    ctx.PacketProduceProtobufPacket(iSockFd, SS_PROTOCOL_MESSAGE_ID_HELLO_RSP, stHelloRsp);
}

template<typename Context>
void HandleSSProtoHelloRsp(Context& ctx, NetPacketBuffer& rstPacket)
{
    ProtoHello::SSHelloRsp stHelloRsp;
    UnpackProtobufStruct(stHelloRsp, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
    LOGINFO("recv pair<{}> hello rsp: {}, diff: {} ms", rstPacket.GetClientFd(), stHelloRsp.timestamp(), (GetMilliSecond()-stHelloRsp.timestamp()));
}

template<typename Context>
void _HandleSSProtoExecute(Context& ctx, NetPacketBuffer& rstPacket)
{
    switch(rstPacket.GetProtoNo())
    {
        case SS_PROTOCOL_MESSAGE_ID_HELLO:
            HandleSSProtoHello(ctx, rstPacket);
            break;
        case SS_PROTOCOL_MESSAGE_ID_HELLO_RSP:
            HandleSSProtoHelloRsp(ctx, rstPacket);
            break;
        default:
            break;
    }
}

void HandleSSProto(ServerContext& rstServerCtxs, NetPacketBuffer& rstPacket)
{
    return _HandleSSProtoExecute(rstServerCtxs, rstPacket);
}

void HandleSSProto(ConnectorContext& rstConnectorCtx, NetPacketBuffer& rstPacket)
{
    return _HandleSSProtoExecute(rstConnectorCtx, rstPacket);
}

void HandleCSProto(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket)
{
    bool bReturnNoNext = true;

    switch(rstPacket.GetProtoNo())
    {
        case CS_PROTOCOL_MESSAGE_ID_HELLO:
            HandleCSProtoHello(rstServerCtx, rstPacket);
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

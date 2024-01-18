#include "handle.h"

using namespace network;

namespace gamehandle
{

void HandleCSProtoHello(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket)
{
    size_t dwLogicTime = GetMilliSecond();
    uint64_t ulClientSeq = rstPacket.GetClientSeq();
    int32_t  iClientFd = rstPacket.GetClientFd();
    ProtoHello::CSHello stHello;
    UnpackProtobufStruct(stHello, rstPacket.GetBuffer(), rstPacket.GetBufferSize());

    ProtoHello::CSHelloRsp stHelloRsp;
    stHelloRsp.set_id(stHello.id());
    stHelloRsp.set_content(stHello.content());
    stHelloRsp.set_unpack_timestamp(rstPacket.GetCreateTime());
    stHelloRsp.set_logic_timestamp(dwLogicTime);
    rstServerCtx.PacketProduceProtobufPacket(ulClientSeq, iClientFd, CS_PROTOCOL_MESSAGE_ID_HELLO, stHelloRsp);
}

template<typename Context>
void HandleSSProtoHello(Context& ctx, NetPacketBuffer& rstPacket)
{
    uint64_t ulClientSeq = rstPacket.GetClientSeq();
    int32_t  iClientFd = rstPacket.GetClientFd();

    ProtoHello::SSHello stHello;
    UnpackProtobufStruct(stHello, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
    LOGINFO("recv pair<{}><{}><{}> hello", ulClientSeq, iClientFd, ctx.GetClientIP(ulClientSeq));

    ProtoHello::SSHelloRsp stHelloRsp;
    stHelloRsp.set_timestamp(GetMilliSecond());
    ctx.PacketProduceProtobufPacket(ulClientSeq, iClientFd, SS_PROTOCOL_MESSAGE_ID_HELLO_RSP, stHelloRsp);
}

template<typename Context>
void HandleSSProtoHelloRsp(Context& ctx, NetPacketBuffer& rstPacket)
{
    ProtoHello::SSHelloRsp stHelloRsp;
    UnpackProtobufStruct(stHelloRsp, rstPacket.GetBuffer(), rstPacket.GetBufferSize());
    LOGINFO("recv pair<{}><{}><{}> hello rsp, diff: {} ms", rstPacket.GetClientSeq(), rstPacket.GetClientFd(),
                        ctx.GetClientIP(rstPacket.GetClientSeq()), (GetMilliSecond()-stHelloRsp.timestamp()));
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
    _HandleSSProtoExecute(rstServerCtxs, rstPacket);
}

void HandleSSProto(ConnectorContext& rstConnectorCtx, NetPacketBuffer& rstPacket)
{
    _HandleSSProtoExecute(rstConnectorCtx, rstPacket);
}

void HandleCSProto(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket)
{
    switch(rstPacket.GetProtoNo())
    {
        case CS_PROTOCOL_MESSAGE_ID_HELLO:
            HandleCSProtoHello(rstServerCtx, rstPacket);
            break;
        default:
            break;
    }
}

} // namespace

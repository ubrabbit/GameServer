#pragma once

#include "common/logger.h"
#include "common/utils/time.h"
#include "network/server.h"
#include "network/packet.h"
#include "network/connector.h"

#include "proto/proto.h"
#include "proto/distcpp/hello.pb.h"

using namespace network;

namespace gamehandle
{

void HandleCSProtoHello(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket);

template<typename Context>
void HandleSSProtoHello(Context& ctx, NetPacketBuffer& rstPacket);

template<typename Context>
void _HandleSSProtoExecute(Context& ctx, NetPacketBuffer& rstPacket);
void HandleSSProto(ServerContext& rstServerCtxs, NetPacketBuffer& rstPacket);
void HandleSSProto(ConnectorContext& rstConnectorCtx, NetPacketBuffer& rstPacket);

void HandleCSProto(ServerContext& rstServerCtx, NetPacketBuffer& rstPacket);

} // namespace

#pragma once

#include "common/logger.h"
#include "common/utils/time.h"
#include "network/server.h"
#include "network/packet.h"

#include "proto/proto.h"
#include "proto/distcpp/hello.pb.h"

using namespace network;

namespace gamehandle
{

void HandleCSProtoHello(Server& rstServer, NetPacketBuffer& rstPacket);

void HandleCSProto(Server& rstServer, NetPacketBuffer& rstPacket);

} // namespace

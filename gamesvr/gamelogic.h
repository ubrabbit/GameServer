#pragma once

#include "common/logger.h"
#include "common/singleton.h"
#include "common/utils/time.h"
#include "network/server.h"
#include "network/packet.h"

#include "proto/proto.h"
#include "proto/distcpp/hello.pb.h"

#include "gameconf.h"
#include "common.h"

class GameLogic : public Singleton<GameLogic>
{
public:
    void StartMainLoop(Gamesvr& rstGamesvr);

private:
    void _ProcessOneLoop(Gamesvr& rstGamesvr);
    void _ProcessTimerCallback(Gamesvr& rstGamesvr);
    void _ProcessCSNetworkPacket(Gamesvr& rstGamesvr);
    void _HandleCSProtocol(network::Server& rstServer, network::NetPacketBuffer& rstPacket);
    void _HandleCSProtoHello(network::Server& rstServer, network::NetPacketBuffer& rstPacket);
};


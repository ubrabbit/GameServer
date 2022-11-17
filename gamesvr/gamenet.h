#pragma once

#include "common/logger.h"
#include "common/singleton.h"
#include "network/server.h"
#include "gameconf.h"
#include "common.h"

class GameNetwork : public Singleton<GameNetwork>
{
public:
    void StartMainLoop(Gamesvr& rstGamesvr);
};

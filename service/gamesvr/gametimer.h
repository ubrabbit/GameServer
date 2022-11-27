#pragma once

#include "common/logger.h"
#include "common/timer.h"
#include "common/utils/time.h"
#include "gameconf.h"
#include "common.h"

using namespace gametimer;

class GameLogicTimer : public Singleton<GameLogicTimer>
{
public:
    void StartMainLoop(Gamesvr& rstGamesvr);

private:
    void _ProcessOneLoop(Gamesvr& rstGamesvr);

};

#include "gametimer.h"

using namespace gametimer;


void GameLogicTimer::StartMainLoop(Gamesvr& rstGamesvr)
{
    LOGINFO("gameimer mainloop start.");
    while(true)
    {
        GameTimer::Instance().TimerUpdate();
        SleepMicroSeconds(500);  // 0.5 ms
    }
    LOGINFO("gameimer mainloop finished.");
}

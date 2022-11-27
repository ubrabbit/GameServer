#include "gametimer.h"

using namespace gametimer;


void GameLogicTimer::StartMainLoop(Gamesvr& rstGamesvr)
{
    LOGINFO("gameimer mainloop start.");
    while(true)
    {
        GameTimer::Instance().TimerUpdate();
        SleepMicroSeconds(100);
    }
    LOGINFO("gameimer mainloop finished.");
}

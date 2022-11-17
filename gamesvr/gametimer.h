#pragma once

#include "common/logger.h"
#include "gameconf.h"
#include "common.h"


void StartTimerLoop(Gamesvr& rstGamesvr)
{
    do{
        LOGINFO("waiting network inited.");
        sleep(1);
    }
    while(false == network::ServerMonitor::Instance().IsInited());
    LOGINFO("wait network inited success.");

}

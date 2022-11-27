#pragma once

#include <stdlib.h>

#include "common/singleton.h"
#include "common/logger.h"
#include "common/conf/config.h"
#include "network/defines.h"


class GameSvrConf : public gameconf::GameConf, public Singleton<GameSvrConf>
{
public:
    struct network::ST_ServerIPInfo m_stServerIPInfo;

    void InitGameSvrConf()
    {
        std::string sConfFilepath("./conf/gamesvr.cfg");
        if(false == Init(sConfFilepath))
        {
            LOGCRITICAL("GameConf init conf filepath failed!");
            return;
        }

        short wPort = getInteger("server", "port", 0);
        std::string sServerIP = get("server", "host", "");
        m_stServerIPInfo.Init(wPort, sServerIP.c_str());

        std::string sLoggerLevel = get("log", "level", "");
        bool bConsole = getBoolean("log", "console", false);
        gamelog::InitGameLogger("gamesvr", sLoggerLevel, bConsole);
    }
};


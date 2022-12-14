#pragma once

#include <stdlib.h>

#include "common/singleton.h"
#include "common/logger.h"
#include "common/conf/config.h"
#include "common/utils/uuid.h"
#include "network/defines.h"


class GameSvrConf : public gameconf::GameConf, public Singleton<GameSvrConf>
{
public:
    struct network::ST_ServerIPInfo m_stServerIPInfo;
    struct network::ST_ServerIPInfo m_stProxyServerIPInfo;
    struct network::ST_ServerIPInfo m_stProxyConnectIPInfo;

    uint32_t    m_dwHostID;
    uint32_t    m_dwGameSvrID;

    void InitGameSvrConf()
    {
        std::string sConfFilepath("./conf/gamesvr.cfg");
        if(false == Init(sConfFilepath))
        {
            LOGCRITICAL("GameConf init conf filepath failed!");
            return;
        }

        m_dwHostID = getInteger("main", "host_id", 0);
        m_dwGameSvrID = getInteger("main", "gamesvr_id", 0);
        SnowFlakeUUID::Instance().Init(m_dwHostID, m_dwGameSvrID);

        short wPort = getInteger("main_server", "port", 0);
        std::string sServerIP = get("main_server", "host", "");
        m_stServerIPInfo.Init(wPort, sServerIP.c_str());

        short wProxyServerPort = getInteger("proxy_server", "port", 0);
        std::string sProxyServerIP = get("proxy_server", "host", "");
        m_stProxyServerIPInfo.Init(wProxyServerPort, sProxyServerIP.c_str());

        short wProxyConnectPort = getInteger("proxy_connect", "port", 0);
        std::string sProxyConnectIP = get("proxy_connect", "host", "");
        m_stProxyConnectIPInfo.Init(wProxyConnectPort, sProxyConnectIP.c_str());

        std::string sLoggerLevel = get("log", "level", "");
        bool bConsole = getBoolean("log", "console", false);
        gamelog::InitGameLogger("gamesvr", sLoggerLevel, bConsole);

    }
};


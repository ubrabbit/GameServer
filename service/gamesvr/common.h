#pragma once

#include "common/conf/config.h"
#include "network/server.h"

class Gamesvr
{
public:
    GameSvrConf     m_stGameSvrConf;
    network::Server m_stMainServer;

};

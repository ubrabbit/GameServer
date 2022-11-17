#include "gamenet.h"


using namespace network;

void GameNetwork::StartMainLoop(Gamesvr& rstGamesvr)
{
    GameSvrConf& rstGameSvrConf = rstGamesvr.m_stGameSvrConf;
    rstGamesvr.m_stMainServer.StartServer(rstGameSvrConf.m_stServerIPInfo);
}

#include "gamenet.h"

using namespace network;

static pthread_t g_ThreadPool[2];


void* ThreadMainServerLoop(void* pstVoid)
{
    assert(pstVoid);
    Gamesvr* pstGamesvr = (Gamesvr*)pstVoid;
    Gamesvr& rstGamesvr = *pstGamesvr;

    GameSvrConf& rstGameSvrConf = rstGamesvr.m_stGameSvrConf;
    LOGINFO("MainServer {} start.", rstGameSvrConf.m_stServerIPInfo.Repr());
    rstGamesvr.m_stMainServer.StartServer(rstGameSvrConf.m_stServerIPInfo);
    LOGINFO("MainServer {} finished.", rstGameSvrConf.m_stServerIPInfo.Repr());

    return NULL;
}


void* ThreadProxyServerLoop(void* pstVoid)
{
    assert(pstVoid);
    Gamesvr* pstGamesvr = (Gamesvr*)pstVoid;
    Gamesvr& rstGamesvr = *pstGamesvr;

    GameSvrConf& rstGameSvrConf = rstGamesvr.m_stGameSvrConf;
    LOGINFO("ProxyServer {} start.", rstGameSvrConf.m_stProxyServerIPInfo.Repr());
    rstGamesvr.m_stProxyServer.StartServer(rstGameSvrConf.m_stProxyServerIPInfo);
    LOGINFO("ProxyServer {} finished.", rstGameSvrConf.m_stProxyServerIPInfo.Repr());

    return NULL;
}


void GameNetwork::StartMainLoop(Gamesvr& rstGamesvr)
{
    memset(&g_ThreadPool, 0, sizeof(g_ThreadPool));
    pthread_create(&g_ThreadPool[0], NULL, ThreadMainServerLoop, &rstGamesvr);
    pthread_create(&g_ThreadPool[1], NULL, ThreadProxyServerLoop, &rstGamesvr);

    if(g_ThreadPool[0] != 0){
        pthread_join(g_ThreadPool[0], NULL);
        LOGINFO("thread main server join finished.");
    }
    if(g_ThreadPool[1] != 0){
        pthread_join(g_ThreadPool[1], NULL);
        LOGINFO("thread proxy server join finished.");
    }

}

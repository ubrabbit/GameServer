/*
This example program provides a trivial server program that listens for TCP
connections on port 9995.  When they arrive, it writes a short message to
each client connection, and closes each connection once it is flushed.
Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include <Python.h>

#include "common/defines.h"
#include "common/logger.h"
#include "common/conf/config.h"
#include "common/signal.h"
#include "gameconf.h"
#include "common.h"
#include "gamelogic.h"
#include "gamenet.h"
#include "gametimer.h"


static pthread_t g_ThreadPool[3];


static void* TaskThreadNetwork(void* pVoidGamesvr)
{
    LOGINFO("thread network started.");
    Gamesvr* pstGamesvr = (Gamesvr*)pVoidGamesvr;
    GameNetwork::Instance().StartMainLoop(*pstGamesvr);
    LOGINFO("thread network finished.");
    return NULL;
}

static void* TaskThreadLogic(void* pVoidGamesvr)
{
    LOGINFO("thread logic started.");
    Gamesvr* pstGamesvr = (Gamesvr*)pVoidGamesvr;
    GameLogic::Instance().StartMainLoop(*pstGamesvr);
    LOGINFO("thread logic finished.");
    return NULL;
}

static void* TaskThreadTimer(void* pVoidGamesvr)
{
    LOGINFO("thread timer started.");
    Gamesvr* pstGamesvr = (Gamesvr*)pVoidGamesvr;
    StartTimerLoop(*pstGamesvr);
    LOGINFO("thread timer finished.");
    return NULL;
}

void OnMainSignalQuit(int sig){
    LOGINFO("catch signal: {}", sig);
    exit(0);
}

int main(int argc, char **argv)
{
    LOGINFO("program start.");
    InitSignalHandle(OnMainSignalQuit);

    Gamesvr* pstGamesvr = new Gamesvr();
    pstGamesvr->m_stGameSvrConf.InitGameSvrConf();

    memset(&g_ThreadPool, 0, sizeof(g_ThreadPool));
    pthread_create(&g_ThreadPool[0], NULL, TaskThreadNetwork, pstGamesvr);
    pthread_create(&g_ThreadPool[1], NULL, TaskThreadLogic, pstGamesvr);
    pthread_create(&g_ThreadPool[2], NULL, TaskThreadTimer, pstGamesvr);

    if(g_ThreadPool[0] != 0){
        pthread_join(g_ThreadPool[0], NULL);
        LOGINFO("thread network join finished.");
    }
    if(g_ThreadPool[1] != 0){
        pthread_join(g_ThreadPool[1], NULL);
        LOGINFO("thread logic join finished.");
    }
    if(g_ThreadPool[2] != 0){
        pthread_join(g_ThreadPool[2], NULL);
        LOGINFO("thread timer join finished.");
    }

    LOGINFO("program quit.");
    return 0;
}

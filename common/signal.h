#pragma once

#include <signal.h>
#include "logger.h"
#include "utils/spinlock.h"

typedef void(*SignaleHandleFunc)(int);

void SignalQuit(int sig);

void InitSignalHandle(SignaleHandleFunc func);

typedef struct ST_GameSingnal
{
public:
    struct spinlock m_stLock;
    bool m_bHasSignal;

    ST_GameSingnal();

    ~ST_GameSingnal();

    void Emit();

    bool CheckAndSetSignalActived();

}ST_GameSingnal;

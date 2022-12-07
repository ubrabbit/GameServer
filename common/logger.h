#pragma once

#include "common/log/bufferlog.h"


#ifndef _GAMELOG_BUFFERLOG_DONOT_USE_KFIFO
    #define LOGTRACE(format, ...)    BUFFER_LOGTRACE(format, ##__VA_ARGS__)
    #define LOGDEBUG(format, ...)    BUFFER_LOGDEBUG(format, ##__VA_ARGS__)
    #define LOGINFO(format, ...)     BUFFER_LOGINFO(format, ##__VA_ARGS__)
    #define LOGWARN(format, ...)     BUFFER_LOGWARN(format, ##__VA_ARGS__)
    #define LOGERROR(format, ...)    BUFFER_LOGERROR(format, ##__VA_ARGS__)
    #define LOGFATAL(format, ...)    BUFFER_LOGFATAL(format, ##__VA_ARGS__)
    #define LOGCRITICAL(format, ...) BUFFER_LOGCRITICAL(format, ##__VA_ARGS__)
#else
    #define LOGTRACE(format, ...)    SPD_LOGTRACE(format, ##__VA_ARGS__)
    #define LOGDEBUG(format, ...)    SPD_LOGDEBUG(format, ##__VA_ARGS__)
    #define LOGINFO(format, ...)     SPD_LOGINFO(format, ##__VA_ARGS__)
    #define LOGWARN(format, ...)     SPD_LOGWARN(format, ##__VA_ARGS__)
    #define LOGERROR(format, ...)    SPD_LOGERROR(format, ##__VA_ARGS__)
    #define LOGFATAL(format, ...)    SPD_LOGFATAL(format, ##__VA_ARGS__)
    #define LOGCRITICAL(format, ...) SPD_LOGCRITICAL(format, ##__VA_ARGS__)
#endif

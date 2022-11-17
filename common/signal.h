#include <signal.h>
#include "logger.h"

typedef void(*SignaleHandleFunc)(int);


void SignalQuit(int sig){
    LOGINFO("catch signal: {}", sig);
    exit(0);
}


void InitSignalHandle(SignaleHandleFunc func){
    /*
    在linux下写socket的程序的时候，如果尝试send到一个disconnected socket上，就会让底层抛出一个SIGPIPE信号。
    这个信号的缺省处理方法是退出进程，大多数时候这都不是我们期望的。因此我们需要重载这个信号的处理方法。
    调用以下代码，即可安全的屏蔽SIGPIPE：
    */
    if(NULL == func)
    {
        func = SignalQuit;
    }

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    /* SIGPIPE 信号屏蔽完毕 */

    /* 注册杀死进程信号 */
    if(signal(SIGINT, func) == SIG_ERR){
        LOGERROR("add signal SIGINT error.");
    }
    if(signal(SIGTERM, func) == SIG_ERR){
        LOGERROR("add signal SIGTERM error.");
    }
    /* 注册杀死进程信号完毕 */
}

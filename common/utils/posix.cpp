#include "posix.h"
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef OS_LINUX

struct in_addr GetHostByName(const std::string &host) {
    struct in_addr addr;
    char buf[1024];
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof hent);
    int r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
    if (r == 0 && he && he->h_addrtype == AF_INET) {
        addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    } else {
        addr.s_addr = INADDR_NONE;
    }
    return addr;
}


uint64_t GetThreadId() {
    return syscall(SYS_gettid);
}


#elif defined(OS_MACOSX)
struct in_addr GetHostByName(const std::string &host) {
    struct in_addr addr;
    struct hostent *he = gethostbyname(host.c_str());
    if (he && he->h_addrtype == AF_INET) {
        addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    } else {
        addr.s_addr = INADDR_NONE;
    }
    return addr;
}

uint64_t GetThreadId() {
    pthread_t tid = pthread_self();
    uint64_t uid = 0;
    memcpy(&uid, &tid, std::min(sizeof(tid), sizeof(uid)));
    return uid;
}

#endif  // #ifdef OS_LINUX

bool CreatePipe(int32_t& iFdRead, int32_t& iFdWrite)
{
    int iPipeFd[2];
	if(pipe(iPipeFd))
    {
		return false;
	}
    iFdRead = iPipeFd[0];
    iFdWrite = iPipeFd[1];

    return true;
}

#pragma once

#include <stdint.h>
#include <errno.h>

static inline int NowDateToInt()
{
	time_t now;
	time(&now);

	// choose thread save version in each platform
	tm p;
#ifdef _WIN32
	localtime_s(&p, &now);
#else
	localtime_r(&now, &p);
#endif // _WIN32
	int now_date = (1900 + p.tm_year) * 10000 + (p.tm_mon + 1) * 100 + p.tm_mday;
	return now_date;
}

static inline int NowTimeToInt()
{
	time_t now;
	time(&now);
	// choose thread save version in each platform
	tm p;
#ifdef _WIN32
	localtime_s(&p, &now);
#else
	localtime_r(&now, &p);
#endif // _WIN32

	int now_int = p.tm_hour * 10000 + p.tm_min * 100 + p.tm_sec;
	return now_int;
}

static inline size_t GetMilliSecond()
{
	size_t t;
#if !defined(__APPLE__) || defined(AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER)
	struct timespec ti;
	clock_gettime(CLOCK_MONOTONIC, &ti);
	t = (size_t)ti.tv_sec * 1000;
	t += ti.tv_nsec / 1000000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = (size_t)tv.tv_sec * 1000;
	t += tv.tv_usec / 1000;
#endif
	return t;
}

static inline void SleepMicroSeconds(int32_t iMicroSeconds){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec= iMicroSeconds;
    int err;
    do{
       err = select(0, NULL, NULL, NULL, &tv);
    }while(err<0 && errno==EINTR);
}

static inline void SleepMilliSeconds(int32_t iMillSeconds){
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec= iMillSeconds * 1000;
    int err;
    do{
       err = select(0, NULL, NULL, NULL, &tv);
    }while(err<0 && errno==EINTR);
}

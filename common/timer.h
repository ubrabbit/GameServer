#pragma once

#include <vector>
#include <map>

#include "noncopyable.h"
#include "singleton.h"
#include "logger.h"
#include "utils/spinlock.h"

namespace gametimer
{

#define GAMETIMER_MAX_TIMER_KEY_LEN (128)
typedef void (*timer_execute_func)(void* arg);

typedef struct ST_TimerCallback
{
public:
	timer_execute_func  	 m_pFuncCallback;
	void*					 m_pstArgs;

	ST_TimerCallback(timer_execute_func func, void* args):
		m_pFuncCallback(func),
		m_pstArgs(args)
	{}

}ST_TimerCallback;

class TimerQueue;
typedef struct ST_TimerContext
{
public:
	uint32_t    			 		m_dwHandle;
	ST_TimerCallback		 		m_stCallback;
	TimerQueue&						m_rstQueue;

	ST_TimerContext(uint32_t dwHandle, ST_TimerCallback& rstCallback, TimerQueue& rstQueue):
		m_dwHandle(dwHandle),
		m_stCallback(rstCallback),
		m_rstQueue(rstQueue)
	{}

	void Timeout();

	void Callback();

}ST_TimerContext;


#define GAMETIMER_TIMER_QUEUE_DEFAULT_SIZE (256)
class TimerQueue
{
public:

	struct spinlock 			  m_stLock;
	std::vector<ST_TimerContext>  m_vecQueue;

	TimerQueue()
	{
		spinlock_init(&m_stLock);
		m_vecQueue.reserve(GAMETIMER_TIMER_QUEUE_DEFAULT_SIZE);
	}

	~TimerQueue()
	{
		spinlock_destroy(&m_stLock);
		m_vecQueue.clear();
	}

	void Init();

	void PushQueue(ST_TimerContext& rstCtx);

	void PrepareExecute(TimerQueue& rstQueue);

};

struct timer;
class GameTimer : public Singleton<GameTimer>
{
public:
	uint32_t								m_dwCounter;
	struct spinlock 						m_stLock;
	struct timer*   						m_pstTimer;

	GameTimer():
		m_dwCounter(0),
		m_pstTimer(NULL)
	{
		spinlock_init(&m_stLock);
	}

	~GameTimer()
	{
		if(NULL != m_pstTimer)
		{
			free(m_pstTimer);
			m_pstTimer = NULL;
		}
		spinlock_destroy(&m_stLock);
	}

	void 	 TimerInit(void);
	void     TimerUpdate(void);

	int 	 AddTimer(std::string& sTimerKey, int time, timer_execute_func func, void* args, TimerQueue& rstQueue);
	int 	 AddTimer(const char* pchTimerKey, int time, timer_execute_func func, void* args, TimerQueue& rstQueue);

	int      RemoveTimer(std::string& sTimerKey);
	int      RemoveTimer(const char* pchTimerKey);
	void 	 TimerTimeout(uint32_t handle, ST_TimerContext* pstTimerCtx);

private:

	std::map<uint32_t, ST_TimerContext*> 	m_mContextMap;
	std::map<std::string, uint32_t> 		m_mTimerKeyMap;

	void 	_AddTimerHandle(uint32_t handle, int time, ST_TimerContext* pstTimerCtx);
	void 	_RemoveTimerHandle(uint32_t handle);

	uint32_t _GenerateHandleID();

};

} // namespace gametimer

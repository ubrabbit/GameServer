/*
The MIT License (MIT)

Copyright (c) 2013 codingnow.com Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://github.com/cloudwu/skynet
*/

#include "timer.h"

#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

namespace gametimer
{

#define TIMER_MALLOC malloc
#define TIMER_FREE free

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)


struct timer_event {
	uint32_t 		  handle;
	ST_TimerContext*  ctx;
};

struct timer_node {
	struct timer_node *next;
	uint32_t expire;
};

struct link_list {
	struct timer_node head;
	struct timer_node *tail;
};

struct timer {
	struct link_list near[TIME_NEAR];
	struct link_list t[4][TIME_LEVEL];
	struct spinlock lock;
	uint32_t time;
	uint32_t starttime;
	uint64_t current;
	uint64_t current_point;
};

static inline struct timer_node *
link_clear(struct link_list *list) {
	struct timer_node * ret = list->head.next;
	list->head.next = 0;
	list->tail = &(list->head);

	return ret;
}

static inline void
link(struct link_list *list,struct timer_node *node) {
	list->tail->next = node;
	list->tail = node;
	node->next = 0;
}

static void
add_node(struct timer *T,struct timer_node *node) {
	uint32_t time=node->expire;
	uint32_t current_time=T->time;

	if ((time|TIME_NEAR_MASK)==(current_time|TIME_NEAR_MASK)) {
		link(&T->near[time&TIME_NEAR_MASK],node);
	} else {
		int i;
		uint32_t mask=TIME_NEAR << TIME_LEVEL_SHIFT;
		for (i=0;i<3;i++) {
			if ((time|(mask-1))==(current_time|(mask-1))) {
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
		}

		link(&T->t[i][((time>>(TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);
	}
}

static void
timer_add(struct timer *T,void *arg,size_t sz,int time) {
	struct timer_node *node = (struct timer_node *)TIMER_MALLOC(sizeof(*node)+sz);
	memcpy(node+1,arg,sz);

	SPIN_LOCK(T);

	node->expire=time+T->time;
	add_node(T,node);

	SPIN_UNLOCK(T);
}

static void
move_list(struct timer *T, int level, int idx) {
	struct timer_node *current = link_clear(&T->t[level][idx]);
	while (current) {
		struct timer_node *temp=current->next;
		add_node(T,current);
		current=temp;
	}
}

static struct timer *
timer_create_timer() {
	struct timer *r=(struct timer *)TIMER_MALLOC(sizeof(struct timer));
	memset(r,0,sizeof(*r));

	int i,j;

	for (i=0;i<TIME_NEAR;i++) {
		link_clear(&r->near[i]);
	}

	for (i=0;i<4;i++) {
		for (j=0;j<TIME_LEVEL;j++) {
			link_clear(&r->t[i][j]);
		}
	}

	SPIN_INIT(r)

	r->current = 0;

	return r;
}

static inline void timer_shift(struct timer *T) {
	int mask = TIME_NEAR;
	uint32_t ct = ++T->time;
	if (ct == 0) {
		move_list(T, 3, 0);
	} else {
		uint32_t time = ct >> TIME_NEAR_SHIFT;
		int i=0;

		while ((ct & (mask-1))==0) {
			int idx=time & TIME_LEVEL_MASK;
			if (idx!=0) {
				move_list(T, i, idx);
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
			time >>= TIME_LEVEL_SHIFT;
			++i;
		}
	}
}

static inline void dispatch_list(GameTimer& rstTimer, struct timer_node *current) {
	do {
		struct timer_event * event = (struct timer_event *)(current+1);
		rstTimer.TimerTimeout(event->handle, event->ctx);

		struct timer_node * temp = current;
		current=current->next;
		TIMER_FREE(temp);
	} while (current);
}

static inline void timer_execute(GameTimer& rstTimer) {
	int idx = rstTimer.m_pstTimer->time & TIME_NEAR_MASK;

	while (rstTimer.m_pstTimer->near[idx].head.next) {
		struct timer_node *current = link_clear(&rstTimer.m_pstTimer->near[idx]);
		SPIN_UNLOCK(rstTimer.m_pstTimer);
		// dispatch_list don't need lock T
		dispatch_list(rstTimer, current);
		SPIN_LOCK(rstTimer.m_pstTimer);
	}
}

void timer_update(GameTimer& rstTimer) {
	SPIN_LOCK(rstTimer.m_pstTimer);

	// try to dispatch timeout 0 (rare condition)
	timer_execute(rstTimer);

	// shift time first, and then dispatch timer message
	timer_shift(rstTimer.m_pstTimer);

	timer_execute(rstTimer);

	SPIN_UNLOCK(rstTimer.m_pstTimer);
}

// centisecond: 1/100 second
static void
systime(uint32_t *sec, uint32_t *cs) {
	struct timespec ti;
	clock_gettime(CLOCK_REALTIME, &ti);
	*sec = (uint32_t)ti.tv_sec;
	*cs = (uint32_t)(ti.tv_nsec / 10000000);
}

static uint64_t
gettime() {
	uint64_t t;
	struct timespec ti;
	clock_gettime(CLOCK_MONOTONIC, &ti);
	t = (uint64_t)ti.tv_sec * 100;
	t += ti.tv_nsec / 10000000;
	return t;
}

void GameTimer::_AddTimerHandle(uint32_t handle, int time, ST_TimerContext* pstTimerCtx)
{
	if (time <= 0) {
		TimerTimeout(handle, pstTimerCtx);
		return;
	} else {
		struct timer_event event;
		event.handle = handle;
		event.ctx = pstTimerCtx;
		timer_add(m_pstTimer, &event, sizeof(event), time);
	}
	return;
}

void GameTimer::_RemoveTimerHandle(uint32_t handle)
{
	auto iter = m_mContextMap.find(handle);
	while(iter != m_mContextMap.end())
	{
		ST_TimerContext* pstTimerCtx = iter->second;
		if(NULL != pstTimerCtx)
		{
			delete pstTimerCtx;
		}
		m_mContextMap.erase(iter);
		iter = m_mContextMap.find(handle);
	}
}

void GameTimer::TimerTimeout(uint32_t handle, ST_TimerContext* pstTimerCtx)
{
	assert(pstTimerCtx);

	spinlock_lock(&m_stLock);
	auto iter = m_mContextMap.find(handle);
	if(iter != m_mContextMap.end())
	{
		pstTimerCtx->Timeout();
	}
	else
	{
		LOGINFO("定时器 {} 回调，但已被删除", handle);
	}
	_RemoveTimerHandle(handle);
	spinlock_unlock(&m_stLock);
}

uint32_t GameTimer::_GenerateHandleID()
{
	m_dwCounter++;
	auto it = m_mContextMap.find(m_dwCounter);
	while(it != m_mContextMap.end() || m_dwCounter >= UINT32_MAX){
		if(m_dwCounter >= UINT32_MAX)
		{
			m_dwCounter = 0;
		}
		m_dwCounter++;
		it = m_mContextMap.find(m_dwCounter);
	};
	return m_dwCounter;
}

int GameTimer::AddTimer(std::string& sTimerKey, int time, timer_execute_func func, void* args, TimerQueue& rstQueue)
{
	if(GAMETIMER_MAX_TIMER_KEY_LEN < sTimerKey.length())
	{
		LOGERROR("AddTimer but key<{}> is too long!", sTimerKey);
		return -2;
	}
	ST_TimerCallback stCallback(func, args);

	spinlock_lock(&m_stLock);

	//同一个key的定时器会被顶掉
	auto it = m_mTimerKeyMap.find(sTimerKey);
	if(it != m_mTimerKeyMap.end())
	{
		LOGINFO("删除老定时器: {}", it->second);
		_RemoveTimerHandle(it->second);
	}

	uint32_t dwHandleID = _GenerateHandleID();
	LOGINFO("添加新定时器: {}", dwHandleID);
	ST_TimerContext* pstTimerCtx = new ST_TimerContext(dwHandleID, stCallback, rstQueue);
	m_mContextMap.insert(std::pair<uint32_t, ST_TimerContext*>(dwHandleID, pstTimerCtx));
	m_mTimerKeyMap.insert(std::pair<std::string, uint32_t>(sTimerKey, dwHandleID));
	_AddTimerHandle(dwHandleID, time, pstTimerCtx);

	spinlock_unlock(&m_stLock);
	return 0;
}

int GameTimer::AddTimer(const char* pchTimerKey, int time, timer_execute_func func, void* args, TimerQueue& rstQueue)
{
	if(NULL == pchTimerKey)
	{
		LOGFATAL("AddTimer but pchTimerKey is NULL! time: {}", time);
		return -1;
	}
	std::string sKey(pchTimerKey);
	return AddTimer(sKey, time, func, args, rstQueue);
}

int GameTimer::RemoveTimer(std::string& sTimerKey)
{
	if(GAMETIMER_MAX_TIMER_KEY_LEN < sTimerKey.length())
	{
		LOGERROR("RemoveTimer but key<{}> is too long!", sTimerKey);
		return -2;
	}

	spinlock_lock(&m_stLock);

	auto it = m_mTimerKeyMap.find(sTimerKey);
	if(it != m_mTimerKeyMap.end())
	{
		_RemoveTimerHandle(it->second);
	}
	m_mTimerKeyMap.erase(sTimerKey);

	spinlock_unlock(&m_stLock);
	return 0;
}

int GameTimer::RemoveTimer(const char* pchTimerKey)
{
	if(NULL == pchTimerKey)
	{
		LOGFATAL("RemoveTimer but pchTimerKey is NULL!");
		return -1;
	}
	std::string sKey(pchTimerKey);
	return RemoveTimer(sKey);
}

void GameTimer::TimerUpdate(void)
{
	assert(m_pstTimer);

	uint64_t cp = gettime();
	if(cp < m_pstTimer->current_point)
	{
		LOGERROR("time diff error: change from {} to {}", cp, m_pstTimer->current_point);
		m_pstTimer->current_point = cp;
	}
	else if (cp != m_pstTimer->current_point)
	{
		uint32_t diff = (uint32_t)(cp - m_pstTimer->current_point);
		m_pstTimer->current_point = cp;
		m_pstTimer->current += diff;
		for (uint32_t i=0;i<diff;i++) {
			timer_update(*this);
		}
	}
}

void GameTimer::TimerInit(void)
{
	m_pstTimer = timer_create_timer();
	uint32_t current = 0;
	systime(&m_pstTimer->starttime, &current);
	m_pstTimer->current = current;
	m_pstTimer->current_point = gettime();
}

void ST_TimerContext::Timeout()
{
	LOGINFO("定时器回调: {}", m_dwHandle);
	m_rstQueue.PushQueue(*this);
}

void ST_TimerContext::Callback()
{
	m_stCallback.m_pFuncCallback(m_stCallback.m_pstArgs);
}

void TimerQueue::Init()
{
	m_vecQueue.clear();
	m_vecQueue.reserve(GAMETIMER_TIMER_QUEUE_DEFAULT_SIZE);
}

void TimerQueue::PushQueue(ST_TimerContext& rstCtx)
{
	spinlock_lock(&m_stLock);
	m_vecQueue.push_back(rstCtx);
	spinlock_unlock(&m_stLock);
}

void TimerQueue::PrepareExecute(TimerQueue& rstQueue)
{
	spinlock_lock(&m_stLock);
	m_vecQueue.swap(rstQueue.m_vecQueue);
	Init();
	spinlock_unlock(&m_stLock);
}

} // namespace gametimer

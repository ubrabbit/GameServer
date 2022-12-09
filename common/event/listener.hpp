#pragma once

#include <errno.h>
#include <map>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include "common/singleton.h"
#include "common/logger.h"
#include "defines.h"
#include "event.hpp"

namespace gameevent
{

class GameEventListener : public Singleton<GameEventListener>
{
public:

    GameEventListener()
    {
        struct event_config* pstEventConfig = event_config_new();
        if(NULL == pstEventConfig)
        {
            LOGCRITICAL("event_config_new fail!");
            return;
        }
        event_config_avoid_method(pstEventConfig, "select");
        event_config_avoid_method(pstEventConfig, "poll");
        m_pstEventBase = event_base_new_with_config(pstEventConfig);
        if(NULL == m_pstEventBase)
        {
            LOGCRITICAL("event_base_new_with_config fail!");
            return;
        }
    }

    ~GameEventListener()
    {
        event_base_loopbreak(m_pstEventBase);
        event_base_free(m_pstEventBase);
        m_pstEventBase = NULL;
    }

    void ProcessOnceLoop()
    {
        event_base_loop(m_pstEventBase, EVLOOP_NONBLOCK | EVLOOP_ONCE);
    }

    bool TriggerEvent(E_GAME_EVENT_TYPE iEventType)
    {
        auto it = m_stEventMap.find(iEventType);
        if(it != m_stEventMap.end())
        {
            GameEvent* pstEvent = it->second;
            pstEvent->Emit();
            return true;
        }
        return false;
    }

    bool RegistEvent(E_GAME_EVENT_TYPE iEventType, evutil_socket_t iSocketFd, int32_t iTimeout, event_callback_fn callback_func)
    {
        GameEvent* pstEvent = new GameEvent(m_pstEventBase, iEventType, iSocketFd, iTimeout, callback_func);
        if(false == AddEvent(pstEvent))
        {
            delete pstEvent;
            return false;
        }
        return true;
    }

    bool AddEvent(GameEvent* pstEvent)
    {
        assert(pstEvent);
        E_GAME_EVENT_TYPE iEventType = pstEvent->m_iEventType;
        auto it = m_stEventMap.find(iEventType);
        if(it != m_stEventMap.end())
        {
            LOGERROR("AddEvent event<{}> fail by already in pool.", iEventType);
            return false;
        }

        LOGINFO("AddEvent event<{}> success", iEventType);
        m_stEventMap.insert(std::pair<E_GAME_EVENT_TYPE, GameEvent*>(iEventType, pstEvent));
        return true;
    }

    bool RemoveEvent(E_GAME_EVENT_TYPE iEventType)
    {
        auto it = m_stEventMap.find(iEventType);
        if(it != m_stEventMap.end())
        {
			GameEvent* pstEvent = it->second;
			m_stEventMap.erase(it);
            delete pstEvent;
			return true;
        }
        return false;
    }

private:
    struct event_base* m_pstEventBase;
    std::map<E_GAME_EVENT_TYPE, GameEvent*> m_stEventMap;

};

} // namespace gameevent

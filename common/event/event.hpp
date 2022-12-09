#pragma once

#include <event2/event.h>

namespace gameevent
{

class GameEvent
{
public:
    E_GAME_EVENT_TYPE   m_iEventType;
    evutil_socket_t     m_iSocketFd;

    int32_t             m_iTimeout;

    GameEvent(struct event_base* pstEventBase, E_GAME_EVENT_TYPE iEventType, evutil_socket_t iSocketFd, int32_t iTimeout, event_callback_fn callback_func):
        m_iEventType(iEventType),
        m_iSocketFd(iSocketFd),
        m_iTimeout(iTimeout)
    {
        assert(pstEventBase);
        m_pstEvent = event_new(pstEventBase, iSocketFd, EV_READ|EV_TIMEOUT|EV_PERSIST, callback_func, (void*)this);

        struct timeval tv;
        tv.tv_sec = m_iTimeout;
        tv.tv_usec = 0;
        if(event_add(m_pstEvent, &tv) < 0){
            LOGCRITICAL("AddEvent event<{}> by error:<{}>!", iEventType, strerror(errno));
            return;
        }

        if(NULL == m_pstEvent)
        {
            LOGCRITICAL("NewEvent event<{}> fail.!", iEventType);
            return;
        }
    }

    ~GameEvent()
    {
        if(NULL != m_pstEvent)
        {
            event_free(m_pstEvent);
            m_pstEvent = NULL;
        }
    }

    void Emit()
    {
        assert(m_pstEvent);
        event_active(m_pstEvent, EV_READ, 0);
    }

private:
    struct event*      m_pstEvent;

};

} // namespace gameevent

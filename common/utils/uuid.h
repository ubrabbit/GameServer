#pragma once

#include "common/singleton.h"
#include "time.h"

class SnowFlakeUUID : public Singleton<SnowFlakeUUID>
{
public:

    SnowFlakeUUID():
        m_ulSequenceID(0),
        m_ulLastTimestamp(0)
    {}

    ~SnowFlakeUUID() = default;

    void Init(uint32_t dwHostID, uint32_t dwGameSvrID)
    {
        m_dwHostID = dwHostID;
        m_dwGameSvrID = dwGameSvrID;
    }

    inline uint64_t _WaitNextTimestamp(uint64_t ulLastTimestamp) {
        uint64_t ulNowTimestamp = (uint64_t)GetMicroSecond();
        while (ulNowTimestamp <= ulLastTimestamp)
        {
            ulNowTimestamp = (uint64_t)GetMicroSecond();
        }
        return ulNowTimestamp;
    }

    uint64_t GenerateSeqID()
    {
        size_t ulNowTimestamp = (uint64_t)GetMicroSecond();
        if (m_ulLastTimestamp == ulNowTimestamp)
        {
            m_ulSequenceID = (m_ulSequenceID + 1) & 0xFFF;
            if (m_ulSequenceID == 0)
            {
                ulNowTimestamp = _WaitNextTimestamp(m_ulLastTimestamp);
            }
        } else {
            m_ulSequenceID = 0;
        }

        m_ulLastTimestamp = ulNowTimestamp;
        return ((ulNowTimestamp & 0x1FFFFFF) << 22 |
                (m_dwHostID & 0x1F) << 17 |
                (m_dwGameSvrID& 0x1F) << 12 |
                (m_ulSequenceID & 0xFFF));
    }

private:

    uint32_t m_dwHostID;
    uint32_t m_dwGameSvrID;
    size_t   m_ulSequenceID;
    size_t   m_ulLastTimestamp;

};

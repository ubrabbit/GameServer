#include "bufferlog.h"

namespace gamelog{

DECLARE_KFIFO(g_stKFiFo, char, BUFFER_LOG_SPACE_SIZE);

void BufferLogKFiFo::_DoLog(int32_t iLogType, int32_t iLine, const char* pchFileName, const char* pchFuncName, const char* pchLogStr)
{
    assert(pchFileName);
    assert(pchFuncName);
    assert(pchLogStr);
    BUFFERLOG_SPDLOG_CALL(gamelog::GameLogger::Instance().GetLogger().get(), pchFileName, iLine, pchFuncName, iLogType, pchLogStr);
}

void BufferLogKFiFo::_DoLog(int32_t iLogType, const char* pchLogStr)
{
    assert(pchLogStr);
    SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), static_cast<spdlog::level::level_enum>(iLogType), pchLogStr);
}

int32_t BufferLogKFiFo::_DoKFiFoPushMsg(ST_BufferLogBlock& rstBufferLogBlock, char(&chBufferArray)[BUFFER_LOG_BLOCK_MAX_SIZE])
{
    if(false == m_bInited)
    {
        _DoLog(rstBufferLogBlock.m_iLogType, chBufferArray);
        return 0;
    }

    if (rstBufferLogBlock.m_iMsgLen > 0)
    {
        if (kfifo_avail(&g_stKFiFo) > (rstBufferLogBlock.m_iMsgLen + sizeof(rstBufferLogBlock)))
        {
            kfifo_in(&g_stKFiFo, (char*)&rstBufferLogBlock, sizeof(rstBufferLogBlock));
            kfifo_in(&g_stKFiFo, chBufferArray, rstBufferLogBlock.m_iMsgLen);
        }
        // 日志队列满，无法写日志
        else
        {
            SPD_LOGFATAL("kfifo buffer is full!");
        }
    }
    return 0;
}

int32_t BufferLogKFiFo::_DoKFiFoPopMsg(ST_BufferLogBlock& rstBufferLogBlock, char(&chTempRecvBuffer)[BUFFER_LOG_BLOCK_MAX_SIZE])
{
    // 队列还没有可读数据
    if (kfifo_len(&g_stKFiFo) < sizeof(rstBufferLogBlock))
    {
        return 0;
    }

    kfifo_out_peek(&g_stKFiFo, (char*)&rstBufferLogBlock, sizeof(rstBufferLogBlock));

    // 数据乱了
    if (rstBufferLogBlock.m_ullStartToken != E_BUFFER_LOG_TOKEN_START
        || rstBufferLogBlock.m_ullEndToken != E_BUFFER_LOG_TOKEN_END
        || (uint32_t)rstBufferLogBlock.m_iMsgLen < 0
        || rstBufferLogBlock.m_iMsgLen >= BUFFER_LOG_BLOCK_MAX_SIZE)
    {
        kfifo_reset_out(&g_stKFiFo);
        SPD_LOGFATAL("kfifo_out_peek token start or end or msg len invalid, token:({}/{})", rstBufferLogBlock.m_ullStartToken, rstBufferLogBlock.m_ullEndToken);
        return -1;
    }

    //数据头发送完了，数据体还没发送完
    if (kfifo_len(&g_stKFiFo) < (uint32_t)(rstBufferLogBlock.m_iMsgLen + sizeof(rstBufferLogBlock)))
    {
        return 0;
    }

    char temp[sizeof(rstBufferLogBlock)] = {0};
    kfifo_out(&g_stKFiFo, temp, sizeof(temp));
    switch (rstBufferLogBlock.m_iLogCmd)
    {
        case E_BUFFER_LOG_CMD_LOG:
            kfifo_out(&g_stKFiFo, chTempRecvBuffer, rstBufferLogBlock.m_iMsgLen);
            chTempRecvBuffer[rstBufferLogBlock.m_iMsgLen] = '\0';
            break;
        default:
            break;
    }

    return rstBufferLogBlock.m_iMsgLen;
}

void BufferLogKFiFo::DoDumpLogToDisk()
{
    ST_BufferLogBlock stBufferLogBlock;
    static char chTempRecvBuffer[BUFFER_LOG_BLOCK_MAX_SIZE];

    int32_t iThreadIdleCnt = 0;
    int32_t iRet = 0;
    while(true)
    {
        iRet = _DoKFiFoPopMsg(stBufferLogBlock, chTempRecvBuffer);
        if(iRet < 0)
        {
            SPD_LOGFATAL("_DoKFiFoPopMsg error: {}", iRet);
            continue;
        }
        if(0 == iRet)
        {
            iThreadIdleCnt++;
            if(iThreadIdleCnt > 10)
            {
                iThreadIdleCnt = 0;
                SleepMicroSeconds(10);
            }
            continue;
        }

        switch (stBufferLogBlock.m_iLogCmd)
        {
            case E_BUFFER_LOG_CMD_LOG:
                _DoLog(stBufferLogBlock.m_iLogType,
                        stBufferLogBlock.m_iLine,
                        stBufferLogBlock.m_chFileName,
                        stBufferLogBlock.m_chFuncName,
                        chTempRecvBuffer);
                break;
            default:
                SPD_LOGFATAL("invalid log cmd: {}", stBufferLogBlock.m_iLogCmd);
                break;
        }
    }
    return;
}

void BufferLogKFiFo::BindMainThread()
{
    m_stThreadID = std::this_thread::get_id();
    m_bInited = true;
    SPD_LOGINFO("success bind log thread");
}

bool BufferLogKFiFo::IsMainThread()
{
    return m_stThreadID == std::this_thread::get_id();
}

void* ThreadDumpLogToDisk(void *pstArg)
{
    SPD_LOGINFO("log thread begin running...");
    BufferLogKFiFo::Instance().DoDumpLogToDisk();
    return NULL;
}

pthread_t g_iLogThreadID;
bool InitGameLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole)
{
    INIT_KFIFO(g_stKFiFo);
    int32_t iAvailableNum = kfifo_avail(&g_stKFiFo);
    SPD_LOGINFO("init kfifo success, avail: {}", iAvailableNum);

	bool bRet = GameLogger::Instance().InitLogger(sNamePrefix, sLogLevel, bConsole);
    if(false == bRet)
    {
        return bRet;
    }

    #ifndef _GAMELOG_BUFFERLOG_DONOT_USE_KFIFO
        pthread_create(&g_iLogThreadID, NULL, ThreadDumpLogToDisk, NULL);
    #endif

    return bRet;
}

} // namespace

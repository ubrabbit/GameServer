#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <string>


#define MAX_BACKTRACE_SIZE (30)
#define MAX_BACKTRACE_LOG_STR_SIZE (100 * 4096)

#define BACKTRACE_DELETE(ptr) { \
    if(NULL != ptr) { delete ptr; } \
}


class LogBacktrace
{
public:

    LogBacktrace():
        m_iBufferLogIdx(0)
    {
        memset(m_achBufferLogStr, 0, sizeof(m_achBufferLogStr));
    }

    ~LogBacktrace() {}

    const std::string GetLogStr()
    {
        return std::string(m_achBufferLogStr);
    }

    const std::string GetTraceMsg()
    {
        void*  pchBuffer[MAX_BACKTRACE_LOG_STR_SIZE] = {0};

        // 显示函数名需要编译时加上 -rdynamic 选项
        int32_t iStackDepth = backtrace(pchBuffer, MAX_BACKTRACE_SIZE);
        char** pchStackString = backtrace_symbols(pchBuffer, iStackDepth);
        if (NULL == pchStackString)
        {
            _FillLogStr("%s", "error while dump stack trace!\n");
            return GetLogStr();
        }

        _FillLogStr("\n%s\n\t", ">>>>>>>>>>>>>>>> STACK TRACE START >>>>>>>>");
        for (int32_t i = 1; i < iStackDepth; i++)
        {
            _FillLogStr("#%02d ", i - 1);
            _FormatStackStr(pchStackString[i]);
        }
        _FillLogStr("\n%s\n\t", "<<<<<<<<<<<<<<<< STACK TRACE END <<<<<<<<");

        BACKTRACE_DELETE(pchStackString);
        return GetLogStr();
    }

private:
    char m_achBufferLogStr[MAX_BACKTRACE_LOG_STR_SIZE];
    int32_t m_iBufferLogIdx;

    void _FillLogStr(const char* sFormat, ...)
    {
        va_list va;
        va_start(va, sFormat);
        _FillLogStr(sFormat, va);
        va_end(va);
    }

    void _FillLogStr(const char* sFormat, va_list& rValst)
    {
        char* pBufferLog = m_achBufferLogStr;
        int32_t iLeftSpace = MAX_BACKTRACE_LOG_STR_SIZE - m_iBufferLogIdx;
        if (m_iBufferLogIdx > MAX_BACKTRACE_LOG_STR_SIZE - 1024)
        {
            // 剩余空间需要大于 '.......more.......' 字符串
            if(iLeftSpace > 32)
            {
                snprintf(pBufferLog + m_iBufferLogIdx, iLeftSpace, "\n.......more.......");
            }
            m_iBufferLogIdx = MAX_BACKTRACE_LOG_STR_SIZE;
            return;
        }
        m_iBufferLogIdx += vsnprintf(pBufferLog + m_iBufferLogIdx, iLeftSpace, sFormat, rValst);
    }

    void _FormatStackStr(char* pchMsg)
    {
        if (NULL == pchMsg)
        {
            _FillLogStr("NULL\n\t");
            return;
        }
        _FillLogStr("%s", pchMsg);
        _FillLogStr("\n\t");
    }

};

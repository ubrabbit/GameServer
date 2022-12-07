#pragma once

#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>

#include "spdlog/spdlog.h"
#include "common/singleton.h"
#include "common/utils/time.h"
#include "log.hpp"

extern "C"
{
	#include "common/kfifo/kfifo.h"
}

namespace gamelog
{

// 1KB
#define BUFFER_LOG_BLOCK_MAX_SIZE (1024 * 1024)
// 2的X次幂
#define BUFFER_LOG_SPACE_SIZE (64 * 1024 * 1024)

// 函数、文件长度
#define BUFFER_LOG_FILE_FUNC_MAX_LENGTH (128)

typedef enum
{
	E_BUFFER_LOG_CMD_NULL = 0,	    // 无效命令
    E_BUFFER_LOG_CMD_LOG,           // 日志
}E_BUFFER_LOG_CMD_TYPE;

typedef enum
{
    E_BUFFER_LOG_TOKEN_START = 0x123456789,
    E_BUFFER_LOG_TOKEN_END   = 0xfedcba987,
}E_BUFFER_LOG_TOKEN;

typedef enum E_BUFFER_LOG_TYPE
{
	E_BUFFER_LOG_TYPE_TRACE = spdlog::level::trace,
	E_BUFFER_LOG_TYPE_DEBUG = spdlog::level::debug,
	E_BUFFER_LOG_TYPE_INFO  = spdlog::level::info,
	E_BUFFER_LOG_TYPE_WARN  = spdlog::level::warn,
	E_BUFFER_LOG_TYPE_ERROR = spdlog::level::err,
	E_BUFFER_LOG_TYPE_FATAL = spdlog::level::critical,
	E_BUFFER_LOG_TYPE_CRITICAL,
}E_BUFFER_LOG_TYPE;

typedef struct ST_SrcLocation
{
public:
    ST_SrcLocation() = default;
    ST_SrcLocation(const char* pchFile, int32_t iLine, const char* pchFunc):
		m_iLine(iLine),
		m_pchFileName(pchFile),
        m_pchFuncName(pchFunc)
    {}

	int32_t m_iLine{0};
    const char* m_pchFileName{NULL};
    const char* m_pchFuncName{NULL};
}ST_SrcLocation;

typedef struct ST_BufferLogBlock
{
public:
	int32_t 				m_iLogCmd;
	int32_t 				m_iLogType;
	int32_t             	m_iMsgLen;

	E_BUFFER_LOG_TOKEN		m_ullStartToken;
	E_BUFFER_LOG_TOKEN  	m_ullEndToken;

	struct timeval			m_stLogTime;

	int32_t m_iLine;
	char m_chFileName[BUFFER_LOG_FILE_FUNC_MAX_LENGTH];
	char m_chFuncName[BUFFER_LOG_FILE_FUNC_MAX_LENGTH];

	ST_BufferLogBlock()
	{
		memset(this, 0, sizeof(*this));
	}

	void construct(ST_SrcLocation& rstSrcLocation, int32_t iLogCmd, int32_t iLogType, int32_t iMsgLen)
	{
		m_iLogCmd  = iLogCmd;
		m_iLogType = iLogType;
		m_iMsgLen  = iMsgLen;

		m_ullStartToken = E_BUFFER_LOG_TOKEN_START;
		m_ullEndToken = E_BUFFER_LOG_TOKEN_END;
		gettimeofday(&m_stLogTime, NULL);

		m_iLine = rstSrcLocation.m_iLine;

		int32_t iStrLen = 0;
		#define COPY_STRING_TO_ARRAY(dest, src){\
			assert(src); \
			iStrLen = strlen(src); \
			iStrLen = ((uint32_t)iStrLen >= sizeof(dest)) ? (sizeof(dest)-1) : iStrLen; \
			strncpy(dest, src, iStrLen); \
			dest[iStrLen] = '\0'; \
		}
		COPY_STRING_TO_ARRAY(m_chFileName, rstSrcLocation.m_pchFileName);
		COPY_STRING_TO_ARRAY(m_chFuncName, rstSrcLocation.m_pchFuncName);
	}

} ST_BufferLogBlock;

class BufferLogKFiFo : public Singleton<BufferLogKFiFo>
{
public:

	template<typename... Args>
	void Log(ST_SrcLocation stLocation, int32_t iLogType, const char* sFormat, Args &&...args)
	{
		static char chTempBuffer[BUFFER_LOG_BLOCK_MAX_SIZE];

		std::string sLog = fmt::format(sFormat, args...);
		int32_t iSize = sLog.size();
		iSize = (iSize >= sizeof(chTempBuffer)) ? (sizeof(chTempBuffer)-1) : iSize;
		strncpy(chTempBuffer, sLog.c_str(), iSize);
		chTempBuffer[iSize] = '\0';

		static struct ST_BufferLogBlock stBufferLogBlock;
		stBufferLogBlock.construct(stLocation, E_BUFFER_LOG_CMD_LOG, iLogType, iSize);
		_DoKFiFoPushMsg(stBufferLogBlock, chTempBuffer);
		return;
	}

	void DoDumpLogToDisk();

	void BindMainThread();
	bool IsMainThread();

private:

	int32_t _DoKFiFoPushMsg(ST_BufferLogBlock& rstBufferLogBlock, char(&chBufferArray)[BUFFER_LOG_BLOCK_MAX_SIZE]);
	int32_t _DoKFiFoPopMsg(ST_BufferLogBlock& rstBufferLogBlock,  char(&chTempRecvBuffer)[BUFFER_LOG_BLOCK_MAX_SIZE]);

	void _DoLog(int32_t iLogType, int32_t iLine, const char* pchFileName, const char* pchFuncName, const char* pchLogStr);
	void _DoLog(int32_t iLogType, const char* pchLogStr);

	std::thread::id m_stThreadID{0};
	bool m_bInited{false};

};

bool InitGameLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole);

} //namespace

#define BUFFERLOG_FUNCTION static_cast<const char *>(__FUNCTION__)
#define BUFFERLOG_SPDLOG_CALL(logger, file, line, funcname, logtype, ...) (logger)->log(spdlog::source_loc{file, line, funcname}, static_cast<spdlog::level::level_enum>(logtype), ##__VA_ARGS__)
#define BUFFERLOG_LOGGER_CALL(level, format, ...) {\
	if(false == gamelog::BufferLogKFiFo::Instance().IsMainThread()){ \
		BUFFERLOG_SPDLOG_CALL(gamelog::GameLogger::Instance().GetLogger().get(), __FILE__, __LINE__, BUFFERLOG_FUNCTION, level, format, ##__VA_ARGS__); \
	} \
	else{ \
		gamelog::BufferLogKFiFo::Instance().Log(gamelog::ST_SrcLocation{__FILE__, __LINE__, BUFFERLOG_FUNCTION}, level, format, ##__VA_ARGS__); \
	} \
}

#define BUFFER_LOGTRACE(format, ...) BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_TRACE, format, ##__VA_ARGS__)
#define BUFFER_LOGDEBUG(format, ...) BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_DEBUG, format, ##__VA_ARGS__)
#define BUFFER_LOGINFO(format, ...)  BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_INFO, format, ##__VA_ARGS__)

#define BUFFER_LOGWARN(format, ...)  BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_WARN, format, ##__VA_ARGS__)
#define BUFFER_LOGERROR(format, ...) BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_ERROR, format, ##__VA_ARGS__)
#define BUFFER_LOGFATAL(format, ...) BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_FATAL, format, ##__VA_ARGS__)
#define BUFFER_LOGCRITICAL(format, ...) BUFFERLOG_LOGGER_CALL(gamelog::E_BUFFER_LOG_TYPE_CRITICAL, format, ##__VA_ARGS__)

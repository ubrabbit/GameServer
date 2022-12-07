#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <time.h>
#include <chrono>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/fmt.h"

#include "common/singleton.h"
#include "common/utils/time.h"

namespace gamelog{

class GameLogger : public Singleton<GameLogger>
{
public:
	const std::string m_sLogDir = "log";
	std::string m_sLoggerPrefix = "";
	bool m_bOutputToConsole = true;
	std::string m_sLevel = "trace";
	std::string m_sLoggerName = "logger";

	GameLogger()
	{
		InitLogger(m_sLoggerPrefix, m_sLevel, m_bOutputToConsole);
	}

	~GameLogger()
	{
		spdlog::drop_all();
	}

	std::shared_ptr<spdlog::logger> GetLogger()
	{
		return m_pstLogger;
	}

	bool InitLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole)
	{
		m_sLoggerPrefix = sNamePrefix;
		m_sLevel = sLogLevel;
		m_bOutputToConsole = bConsole;

		m_sLoggerName = m_sLoggerPrefix + "_" + m_sLevel;

		try
		{
			if (m_bOutputToConsole)
				m_pstLogger = spdlog::stdout_color_st(m_sLoggerName); // single thread console output faster
			else
				// multi part log files, with every part 500M, max 10 files
				m_pstLogger = spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(
					m_sLoggerName,
					m_sLogDir + "/" + m_sLoggerName + ".log",
					500 * 1024 * 1024, 10);

			// 2022-11-09 09:38:50.626975 <thread 26361> [info] [gamesvr.cpp:89:main()] program start.
			m_pstLogger->set_pattern("%Y-%m-%d %H:%M:%S.%f <thread %t> [%l] [%s:%#:%!()] %v");

			if (m_sLevel == "trace")
			{
				m_pstLogger->set_level(spdlog::level::trace);
				m_pstLogger->flush_on(spdlog::level::trace);
			}
			else if (m_sLevel == "debug")
			{
				m_pstLogger->set_level(spdlog::level::debug);
				m_pstLogger->flush_on(spdlog::level::debug);
			}
			else if (m_sLevel == "info")
			{
				m_pstLogger->set_level(spdlog::level::info);
				m_pstLogger->flush_on(spdlog::level::info);
			}
			else if (m_sLevel == "error")
			{
				m_pstLogger->set_level(spdlog::level::err);
				m_pstLogger->flush_on(spdlog::level::err);
			}
			else if (m_sLevel == "critical")
			{
				m_pstLogger->set_level(spdlog::level::critical);
				m_pstLogger->flush_on(spdlog::level::critical);
			}
			else
			{
				std::cout << "invalid log level: " << m_sLevel << std::endl;
			}
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			std::cout << "Log initialization failed: " << ex.what() << std::endl;
			return false;
		}
		return true;
	}

private:
	std::shared_ptr<spdlog::logger> m_pstLogger;
};

} //namespace

#define SPD_LOGTRACE(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::trace, format, ##__VA_ARGS__)
#define SPD_LOGDEBUG(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::debug, format, ##__VA_ARGS__)
#define SPD_LOGINFO(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::info, format, ##__VA_ARGS__)
#define SPD_LOGWARN(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::warn, format, ##__VA_ARGS__)
#define SPD_LOGERROR(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::err, format, ##__VA_ARGS__)
#define SPD_LOGFATAL(format, ...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::critical, format, ##__VA_ARGS__)
#define SPD_LOGCRITICAL(format, ...) \
do { \
	SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::critical, format, ##__VA_ARGS__); \
	exit(2);\
} while(0)

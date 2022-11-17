#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <time.h>
#include <chrono>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h" // or "../stdout_sinks.h" if no color needed
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "singleton.h"
#include "time.h"

namespace gamelog{

class GameLogger : public Singleton<GameLogger>
{
public:
	const std::string m_sLogDir = "log";
	std::string m_sLoggerPrefix = "";
	bool m_bOutputToConsole = true;
	std::string m_sLevel = "trace";
	std::string m_sLoggerName = "logger";

	std::shared_ptr<spdlog::logger> GetLogger();

	bool InitLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole);

	GameLogger();
	~GameLogger();

private:
	std::shared_ptr<spdlog::logger> m_pstLogger;
};

bool InitGameLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole);

} //namespace

// use embedded macro to support file and line number
#define LOGTRACE(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::trace, __VA_ARGS__)
#define LOGDEBUG(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::debug, __VA_ARGS__)
#define LOGINFO(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::info, __VA_ARGS__)
#define LOGWARN(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::warn, __VA_ARGS__)
#define LOGERROR(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::err, __VA_ARGS__)
#define LOGFATAL(...) SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::critical, __VA_ARGS__)
#define LOGCRITICAL(...) \
do { \
	SPDLOG_LOGGER_CALL(gamelog::GameLogger::Instance().GetLogger().get(), spdlog::level::critical, __VA_ARGS__); \
	exit(2);\
} while(0)

#include "logger.h"

namespace gamelog
{

std::shared_ptr<spdlog::logger> GameLogger::GetLogger()
{
    return m_pstLogger;
}

bool GameLogger::InitLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole)
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

GameLogger::GameLogger()
{
    InitLogger(m_sLoggerPrefix, m_sLevel, m_bOutputToConsole);
}

GameLogger::~GameLogger()
{
    spdlog::drop_all(); // must do this
}

bool InitGameLogger(std::string sNamePrefix, std::string sLogLevel, bool bConsole)
{
	return GameLogger::Instance().InitLogger(sNamePrefix, sLogLevel, bConsole);
}

} //namespace

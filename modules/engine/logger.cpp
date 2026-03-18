#include "logger.h"
#include "raylib.h"

namespace BreadEngine {
    Action<Logger::LogLevel, std::string_view> Logger::OnLog{};

    void Logger::LogInfo(const std::string_view message)
    {
        Log(Info, message);
    }

    void Logger::LogWarning(const std::string_view message)
    {
        Log(Warning, message);
    }

    void Logger::LogError(const std::string_view message)
    {
        Log(Error, message);
    }

    void Logger::Log(const LogLevel &level, const std::string_view message)
    {
        const auto logLevel = level == Info ? LOG_INFO : level == Warning ? LOG_WARNING : LOG_ERROR;
        TraceLog(logLevel, message.data());
        OnLog.invoke(level, message);
    }
} // BreadEngine

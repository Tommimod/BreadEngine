#include "logger.h"
#include "raylib.h"
#include <ctime>

#include "configs/baseYamlConfig.h"
#include "stacktrace.h"

namespace BreadEngine {
    Action<Logger::LogEntity &> Logger::OnLog{};
    std::vector<Logger::LogEntity> Logger::_logs{};

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

    const std::vector<Logger::LogEntity> &Logger::getLogs()
    {
        return _logs;
    }

    void Logger::clear()
    {
        _logs.clear();
    }

    void Logger::Log(const LogLevel &level, const std::string_view message)
    {
        const auto t = std::time(nullptr);
        const auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        const auto dateTime = oss.str();

        const auto logLevel = level == Info ? LOG_INFO : level == Warning ? LOG_WARNING : LOG_ERROR;
        const auto text = TextFormat("[%s] %s \n%s", dateTime.c_str(), message.data(), ("       " + Stacktrace::get_stacktrace_string()).c_str());
        TraceLog(logLevel, text);
        auto entity = LogEntity{.level = level, .message = text};
        OnLog.invoke(entity);
        _logs.emplace_back(std::move(entity));
    }
} // BreadEngine

#pragma once
#include <string_view>

#include "action.h"

namespace BreadEngine {
    class Logger
    {
    public:
        enum LogLevel { Info, Warning, Error };

        struct LogEntity
        {
            LogLevel level;
            std::string_view message;
        };

        static Action<LogEntity &> OnLog;

        static void LogInfo(std::string_view message);

        static void LogWarning(std::string_view message);

        static void LogError(std::string_view message);

        static const std::vector<LogEntity> &getLogs();

        static void clear();

    private:
        static std::vector<LogEntity> _logs;

        static void Log(const LogLevel &level, std::string_view message);
    };
} // BreadEngine

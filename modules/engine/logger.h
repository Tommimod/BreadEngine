#pragma once
#include <string_view>

#include "action.h"

namespace BreadEngine {
    class Logger
    {
    public:
        enum LogLevel { Info, Warning, Error };

        static Action<LogLevel, std::string_view> OnLog;

        static void LogInfo(std::string_view message);

        static void LogWarning(std::string_view message);

        static void LogError(std::string_view message);

    private:
        static void Log(const LogLevel &level, std::string_view message);
    };
} // BreadEngine

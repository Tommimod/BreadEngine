#pragma once
#include <string>
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
            /// Owned, not a view: entries outlive the caller's buffer and are re-read by the
            /// editor's console long after the call that produced them.
            std::string message;
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

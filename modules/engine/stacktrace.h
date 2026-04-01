#pragma once
#include <string>

namespace BreadEngine {
    class Stacktrace
    {
    public:
        static std::string get_stacktrace_string(int max_depth = 32);
    };
}

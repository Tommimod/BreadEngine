#include "stacktrace.h"
#include <backward.hpp>

namespace BreadEngine {
    std::string Stacktrace::get_stacktrace_string(int max_depth, bool with_snippets)
    {
        using namespace backward;
        StackTrace st;
        st.load_here(max_depth);

        Printer p;
        p.object = true;
        p.address = false;
        p.color_mode = ColorMode::never;
        p.snippet = with_snippets;

        std::ostringstream oss;
        p.print(st, oss);
        return oss.str();
    }
} // BreadEngine

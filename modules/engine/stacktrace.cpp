#include "stacktrace.h"
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>
#include <cpptrace/basic.hpp>
#include <sstream>

namespace BreadEngine {

    std::string Stacktrace::get_stacktrace_string(const int max_depth)
    {
        auto trace = cpptrace::generate_trace(0, max_depth);

        auto fmt = cpptrace::formatter{}
        .header("Stack trace:")
        .colors(cpptrace::formatter::color_mode::none)
        .addresses(cpptrace::formatter::address_mode::none)
        .paths(cpptrace::formatter::path_mode::full)
        .symbols(cpptrace::formatter::symbol_mode::pretty)
        .snippets(false);

        std::ostringstream oss;
        fmt.print(oss, trace);
        return oss.str();
    }

} // namespace BreadEngine
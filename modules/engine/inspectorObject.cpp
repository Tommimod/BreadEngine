#include "inspectorObject.h"
#include <cstdlib>
#include <cxxabi.h>

namespace BreadEngine {
    std::string InspectorStruct::getTypeName()
    {
        int status;
        auto *mangled = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
        std::string result = status == 0 ? mangled : typeid(*this).name();
        std::free(mangled);

        if (const size_t lastColon = result.rfind("::"); lastColon != std::string::npos)
        {
            return result.substr(lastColon + 2);
        }
        return result;
    }
} // BreadEngine

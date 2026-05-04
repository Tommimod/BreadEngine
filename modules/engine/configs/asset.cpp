#include "asset.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Asset)

    Asset::Asset(const std::string &guid, const std::string &name)
    {
        _guid = guid;
        _name = name;
    }
} // BreadEngine
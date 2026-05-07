#pragma once
#include <yaml-cpp/node/node.h>

namespace BreadEngine {
    class AssetsDeserializer
    {
    public:
        AssetsDeserializer() = default;

        ~AssetsDeserializer() = default;

        static void deserialize(YAML::Node &node);
    };
} // BreadEngine

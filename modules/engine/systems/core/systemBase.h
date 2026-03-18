#pragma once
#include "node.h"

namespace BreadEngine {
    class SystemBase
    {
    public:
        SystemBase() = default;

        virtual ~SystemBase() = default;

        [[nodiscard]] virtual bool isInitializable() const { return false; }
        [[nodiscard]] virtual bool isUpdatable() const { return false; }
        [[nodiscard]] virtual bool isEndOnFrame() const { return false; }
        [[nodiscard]] virtual bool isValid(Node &node) { return true; }
    };
} // BreadEngine

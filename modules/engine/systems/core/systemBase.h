#pragma once
#include "node.h"

namespace BreadEngine {
    class SystemBase
    {
    public:
        SystemBase() = default;

        virtual ~SystemBase() = default;

        [[nodiscard]] virtual bool isInitialize() const { return false; }
        [[nodiscard]] virtual bool isUpdate() const { return false; }
        [[nodiscard]] virtual bool isFixedUpdate() const { return false; }
        [[nodiscard]] virtual bool isStartOnFrame() const { return false; }
        [[nodiscard]] virtual bool isEndOnFrame() const { return false; }

        [[nodiscard]] virtual bool isValid(Node &node);
    };
} // BreadEngine

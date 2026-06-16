#pragma once
#include "node.h"

namespace BreadEngine {
    class SystemBase
    {
    public:
        SystemBase() = default;
        virtual ~SystemBase() = default;

        static constexpr bool kOnlyRuntime = false;

        [[nodiscard]] virtual bool isValid(const Node *node);

    protected:
        [[nodiscard]] static bool isEngineRuntime() noexcept;
    };

    template<typename T>
    concept HasCustomIsValid = !std::is_same_v<
        decltype(&T::isValid),
        decltype(&SystemBase::isValid)
    >;
} // namespace BreadEngine

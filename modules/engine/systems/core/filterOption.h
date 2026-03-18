#pragma once
#include <array>
#include <stdexcept>
#include <type_traits>

#include "component.h"
#include "componentsProvider.h"
#include "node.h"

namespace BreadEngine {
    struct FilterOption
    {
    private:
        using MaskArray = std::array<uint64_t, ComponentsProvider::MAX_MASK_COUNT>;

        MaskArray _withMasks{};
        MaskArray _withAnyMasks{};
        MaskArray _withoutMasks{};
        int _maskCount = 0;

        template<typename T>
        static size_t getComponentIndex()
        {
            return ComponentsProvider::GetComponentIndex<T>();
        }

        template<typename T>
        void addTypeToMask(MaskArray &masks) const
        {
            static_assert(std::is_base_of_v<Component, T>,
                          "All types must derive from Component");

            const size_t index = getComponentIndex<T>();
            const size_t arrayIndex = index / 64;
            const size_t bitIndex = index % 64;

            if (arrayIndex >= static_cast<size_t>(_maskCount))
            {
                throw std::runtime_error(
                    "Component index out of range. "
                    "Increase maskCount in FilterOption ctor or MAX_MASK_COUNT.");
            }

            masks[arrayIndex] |= 1ULL << bitIndex;
        }

    public:
        static FilterOption empty() noexcept { return FilterOption(ComponentsProvider::MAX_MASK_COUNT); }

        explicit FilterOption(const int maskCount) : _maskCount(maskCount)
        {
        }

        template<typename... Ts>
        [[nodiscard]] FilterOption &with()
        {
            static_assert((std::is_base_of_v<Component, Ts> && ...), "All types must derive from Component");
            (addTypeToMask<Ts>(_withMasks), ...);
            return *this;
        }

        template<typename... Ts>
        [[nodiscard]] FilterOption withAny()
        {
            static_assert((std::is_base_of_v<Component, Ts> && ...));
            (addTypeToMask<Ts>(_withAnyMasks), ...);
            return *this;
        }

        template<typename... Ts>
        [[nodiscard]] FilterOption without() const
        {
            static_assert((std::is_base_of_v<Component, Ts> && ...));
            (addTypeToMask<Ts>(_withoutMasks), ...);
            return *this;
        }

        [[nodiscard]] bool isEquals(const FilterOption &other) const
        {
            if (_maskCount != other._maskCount) return false;
            for (int i = 0; i < _maskCount; ++i)
            {
                if (_withMasks[i] != other._withMasks[i] ||
                    _withAnyMasks[i] != other._withAnyMasks[i] ||
                    _withoutMasks[i] != other._withoutMasks[i])
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool isValid(const Node &node) const
        {
            const auto nodeMask = node.getComponentMasks(_maskCount);
            for (int i = 0; i < _maskCount; ++i)
            {
                const uint64_t am = nodeMask[i];
                if ((_withMasks[i] & am) != _withMasks[i]) return false;
                if (_withAnyMasks[i] != 0 && (_withAnyMasks[i] & am) == 0) return false;
                if ((_withoutMasks[i] & am) != 0) return false;
            }
            return true;
        }

        bool operator==(const FilterOption &other) const noexcept
        {
            return isEquals(other);
        }
    };
} // namespace BreadEngine

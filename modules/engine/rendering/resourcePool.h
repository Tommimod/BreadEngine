#pragma once
#include <cstdint>
#include <deque>
#include <utility>
#include <vector>

namespace BreadEngine {
    /**
     * Slot storage behind the renderer's handles: freed slots are reused, and their
     * generation counter is bumped so handles to the old occupant stop resolving.
     *
     * Slot addresses are stable for the lifetime of the pool - std::deque does not move
     * existing elements when it grows - so background work may hold a pointer to a slot.
     */
    template<typename Slot, typename Handle>
    class ResourcePool
    {
    public:
        Handle add(Slot &&slot)
        {
            if (!_freeIndices.empty())
            {
                const auto index = _freeIndices.back();
                _freeIndices.pop_back();

                auto &entry = _entries[index];
                entry.slot = std::move(slot);
                entry.alive = true;
                return Handle{.index = index, .generation = entry.generation};
            }

            const auto index = static_cast<uint32_t>(_entries.size());
            _entries.push_back(Entry{.slot = std::move(slot), .alive = true});
            return Handle{.index = index, .generation = 0};
        }

        [[nodiscard]] Slot *get(const Handle handle)
        {
            return const_cast<Slot *>(std::as_const(*this).get(handle));
        }

        [[nodiscard]] const Slot *get(const Handle handle) const
        {
            if (!handle.isValid() || handle.index >= _entries.size()) return nullptr;

            const auto &entry = _entries[handle.index];
            if (!entry.alive || entry.generation != handle.generation) return nullptr;

            return &entry.slot;
        }

        /// Releasing whatever the slot owns is the caller's job and must happen before this.
        void remove(const Handle handle)
        {
            if (get(handle) == nullptr) return;

            release(handle.index);
        }

        /**
         * Frees every live slot @p predicate accepts, and hands it a mutable slot so it can
         * settle whatever the slot still owes first. Separate from forEachAlive because a
         * visitor there has no way to name the slot it is standing on, and so no way to free it.
         */
        template<typename Fn>
        void removeIf(Fn &&predicate)
        {
            for (uint32_t index = 0; index < _entries.size(); ++index)
            {
                auto &entry = _entries[index];
                if (!entry.alive || !predicate(entry.slot)) continue;

                release(index);
            }
        }

    private:
        void release(const uint32_t index)
        {
            auto &entry = _entries[index];
            entry.slot = Slot{};
            entry.alive = false;
            ++entry.generation;
            _freeIndices.push_back(index);
        }

    public:
        template<typename Fn>
        void forEachAlive(Fn &&fn)
        {
            for (auto &entry: _entries)
            {
                if (entry.alive) fn(entry.slot);
            }
        }

        void clear()
        {
            _entries.clear();
            _freeIndices.clear();
        }

    private:
        struct Entry
        {
            Slot slot;
            uint32_t generation = 0;
            bool alive = false;
        };

        std::deque<Entry> _entries;
        std::vector<uint32_t> _freeIndices;
    };
} // namespace BreadEngine

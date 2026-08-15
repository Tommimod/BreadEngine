#pragma once
#include <vector>

#include "r3d_lighting.h"
#include "../../IRenderer.h"

namespace BreadEngine {
    /**
     * r3d/raylib implementation of the render seam - the "before" side of the migration.
     *
     * Everything r3d-specific is being funnelled into this one directory so that Phase 9
     * ("remove R3D from engine+game") is a directory deletion plus one CMake branch, not an
     * archaeology exercise across the whole engine.
     */
    class R3DRenderer final : public IRenderer
    {
    public:
        [[nodiscard]] const char *getBackendName() const override { return "R3D"; }

        void initialize() override;

        void shutdown() override;

        // --- lights ---

        [[nodiscard]] LightHandle createLight(LightType type) override;

        void destroyLight(LightHandle handle) override;

        [[nodiscard]] bool isLightValid(LightHandle handle) const override;

        void updateLight(LightHandle handle, const LightState &state) override;

    private:
        /// One pool slot. `applied` is the state already pushed to r3d, so updateLight() can
        /// skip redundant setters exactly the way lightSystem.cpp used to by querying r3d.
        struct LightSlot
        {
            R3D_Light native = -1; ///< r3d light id; negative means invalid
            LightState applied{};
            uint32_t generation = 0;
            bool alive = false;
            bool hasApplied = false; ///< false right after (re)creation - forces a full apply
        };

        std::vector<LightSlot> _lights;
        std::vector<uint32_t> _freeLightSlots;

        [[nodiscard]] LightSlot *resolveLight(LightHandle handle);

        [[nodiscard]] const LightSlot *resolveLight(LightHandle handle) const;

        static R3D_LightType toNative(LightType type);
    };
} // namespace BreadEngine

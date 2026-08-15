#include "r3dRenderer.h"

#include <utility>

#include "utils/colorUtils.h"

namespace BreadEngine {
    void R3DRenderer::initialize()
    {
        // r3d itself is still brought up by Engine::initialize() (R3D_Init) - moving window
        // and device lifetime behind the seam is its own migration sub-phase.
    }

    void R3DRenderer::shutdown()
    {
        for (auto &slot: _lights)
        {
            if (slot.alive && R3D_IsLightExist(slot.native))
            {
                R3D_DestroyLight(slot.native);
            }
        }

        _lights.clear();
        _freeLightSlots.clear();
    }

    R3D_LightType R3DRenderer::toNative(const LightType type)
    {
        switch (type)
        {
            case LightType::Spot: return R3D_LIGHT_SPOT;
            case LightType::Omni: return R3D_LIGHT_OMNI;
            case LightType::Directional:
            default: return R3D_LIGHT_DIR;
        }
    }

    LightHandle R3DRenderer::createLight(const LightType type)
    {
        uint32_t index;
        if (!_freeLightSlots.empty())
        {
            index = _freeLightSlots.back();
            _freeLightSlots.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(_lights.size());
            _lights.emplace_back();
        }

        auto &slot = _lights[index];
        slot.native = R3D_CreateLight(toNative(type));
        slot.applied = LightState{};
        slot.applied.type = type;
        slot.hasApplied = false;
        slot.alive = true;

        return LightHandle{.index = index, .generation = slot.generation};
    }

    void R3DRenderer::destroyLight(const LightHandle handle)
    {
        auto *slot = resolveLight(handle);
        if (slot == nullptr) return;

        if (R3D_IsLightExist(slot->native))
        {
            R3D_DestroyLight(slot->native);
        }

        slot->native = -1;
        slot->alive = false;
        slot->hasApplied = false;
        // Invalidate every handle still pointing here before the slot can be handed out again.
        slot->generation++;
        _freeLightSlots.push_back(handle.index);
    }

    bool R3DRenderer::isLightValid(const LightHandle handle) const
    {
        const auto *slot = resolveLight(handle);
        return slot != nullptr && R3D_IsLightExist(slot->native);
    }

    void R3DRenderer::updateLight(const LightHandle handle, const LightState &state)
    {
        auto *slot = resolveLight(handle);
        if (slot == nullptr) return;

        // A light's type is baked into the r3d object, so a type change means a new one.
        // The pool slot (and therefore the caller's handle) survives it.
        if (!R3D_IsLightExist(slot->native) || R3D_GetLightType(slot->native) != toNative(state.type))
        {
            if (R3D_IsLightExist(slot->native))
            {
                R3D_DestroyLight(slot->native);
            }

            slot->native = R3D_CreateLight(toNative(state.type));
            slot->hasApplied = false;
        }

        const bool forceApply = !slot->hasApplied;
        const LightState &applied = slot->applied;

        if (forceApply || state.castShadows != applied.castShadows)
        {
            if (state.castShadows)
            {
                R3D_EnableShadow(slot->native);
            }
            else
            {
                R3D_DisableShadow(slot->native);
            }
        }

        if (forceApply || !ColorUtils::IsCompare(applied.color, state.color))
        {
            R3D_SetLightColor(slot->native, state.color);
        }

        if (forceApply || applied.range != state.range)
        {
            R3D_SetLightRange(slot->native, state.range);
        }

        if (state.castShadows && (forceApply || applied.shadowSoftness != state.shadowSoftness))
        {
            R3D_SetShadowSoftness(slot->native, state.shadowSoftness);
        }

        if (forceApply || applied.intensity != state.intensity)
        {
            R3D_SetLightEnergy(slot->native, state.intensity);
        }

        if (forceApply || applied.active != state.active)
        {
            R3D_SetLightActive(slot->native, state.active);
        }

        // Direction and position stay unconditional, matching the pre-seam lightSystem:
        // r3d may key shadow-map refreshes off these setters, so skipping a redundant call
        // is not provably a no-op the way the diffed scalars above are.
        if (state.type != LightType::Omni)
        {
            R3D_SetLightDirection(slot->native, state.direction);
        }

        if (state.type != LightType::Directional)
        {
            R3D_SetLightPosition(slot->native, state.position);
        }

        slot->applied = state;
        slot->hasApplied = true;
    }

    R3DRenderer::LightSlot *R3DRenderer::resolveLight(const LightHandle handle)
    {
        return const_cast<LightSlot *>(std::as_const(*this).resolveLight(handle));
    }

    const R3DRenderer::LightSlot *R3DRenderer::resolveLight(const LightHandle handle) const
    {
        if (!handle.isValid() || handle.index >= _lights.size()) return nullptr;

        const auto &slot = _lights[handle.index];
        if (!slot.alive || slot.generation != handle.generation) return nullptr;

        return &slot;
    }
} // namespace BreadEngine

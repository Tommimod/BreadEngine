#pragma once
#include "inspectorObject.h"
#include "r3d_environment.h"

namespace BreadEngine {
    struct EnvironmentAmbientParameters : InspectorStruct
    {
        Color color = WHITE; ///< Ambient light color when there is no ambient map
        float energy = 0; ///< Energy multiplier for ambient light (map or color)
        R3D_AmbientMap map{}; ///< IBL environment map, can be generated from skybox

        EnvironmentAmbientParameters() = default;

        ~EnvironmentAmbientParameters() override = default;

        EnvironmentAmbientParameters &fromNative(const R3D_EnvAmbient &nativeData);

        [[nodiscard]] R3D_EnvAmbient toNative() const;

        void generateFromCubemap(const R3D_Cubemap &cubemap);

        void clear();

    private:
        INSPECTOR_BEGIN(EnvironmentAmbientParameters)
            INSPECT_FIELD(color)
            INSPECT_FIELD(energy)
        INSPECTOR_END()
    };
} // BreadEngine

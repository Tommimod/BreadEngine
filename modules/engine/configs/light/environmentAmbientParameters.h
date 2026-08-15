#pragma once
#include "inspectorObject.h"
#include "rendering/renderTypes.h"

namespace BreadEngine {
    struct EnvironmentAmbientParameters : InspectorStruct
    {
        Color color = WHITE; ///< Ambient light color when there is no ambient map
        float energy = 0; ///< Energy multiplier for ambient light (map or color)
        AmbientMapHandle map{}; ///< IBL environment map, generated from the skybox

        EnvironmentAmbientParameters() = default;

        ~EnvironmentAmbientParameters() override = default;

        void generateFromCubemap(CubemapHandle cubemap);

        void clear();

    private:
        INSPECTOR_BEGIN(EnvironmentAmbientParameters)
            INSPECT_FIELD(color)
            INSPECT_FIELD(energy)
        INSPECTOR_END()
    };
} // BreadEngine

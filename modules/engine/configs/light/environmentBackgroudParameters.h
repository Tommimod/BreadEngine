#pragma once
#include "inspectorObject.h"
#include "skyboxCubemapParameters.h"
#include "rendering/renderTypes.h"
#include "configs/assets/textureAsset.h"

namespace BreadEngine {
    struct EnvironmentBackgroudParameters : InspectorStruct
    {
        CubemapHandle sky{}; ///< Skybox cubemap; invalid means the flat colour is used
        Quaternion rotation{0, 0, 0, 1}; ///< Skybox rotation (pitch, yaw, roll as quaternion)
        Color color = GRAY; ///< Background color when there is no skybox
        float energy = 1.0f; ///< Energy multiplier applied to background (skybox or color)
        float skyBlur = 0; ///< Sky blur factor [0,1], based on mipmaps, very fast

        EnvironmentBackgroudParameters() = default;

        ~EnvironmentBackgroudParameters() override = default;

        void setTexture(TextureAsset *texture, const SkyboxCubemapParameters &settings);

        void clearTexture();

    private:
        INSPECTOR_BEGIN(EnvironmentBackgroudParameters)
            INSPECT_FIELD(skyBlur)
            INSPECT_FIELD(energy)
            INSPECT_FIELD(color)
            INSPECT_FIELD(rotation)
        INSPECTOR_END()
    };
} // BreadEngine

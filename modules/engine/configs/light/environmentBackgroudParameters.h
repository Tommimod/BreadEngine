#pragma once
#include "inspectorObject.h"
#include "r3d_cubemap.h"
#include "r3d_environment.h"
#include "configs/assets/textureAsset.h"

namespace BreadEngine {
    struct EnvironmentBackgroudParameters : InspectorStruct
    {
        R3D_Cubemap sky{}; ///< Skybox asset (used if ID is non-zero)
        Quaternion rotation{}; ///< Skybox rotation (pitch, yaw, roll as quaternion)
        Color color = WHITE; ///< Background color when there is no skybox
        float energy = 0; ///< Energy multiplier applied to background (skybox or color)
        float skyBlur = 0; ///< Sky blur factor [0,1], based on mipmaps, very fast

        EnvironmentBackgroudParameters() = default;

        ~EnvironmentBackgroudParameters() override = default;

        EnvironmentBackgroudParameters &fromNative(const R3D_EnvBackground &nativeData);

        [[nodiscard]] R3D_EnvBackground toNative() const;

        void setTexture(TextureAsset *texture);

        void clearTexture();

    private:
        TextureAsset *_skyboxTexture = nullptr;

        INSPECTOR_BEGIN(EnvironmentBackgroudParameters)
            INSPECT_FIELD(_skyboxTexture)
            INSPECT_FIELD(skyBlur)
            INSPECT_FIELD(energy)
            INSPECT_FIELD(color)
            INSPECT_FIELD(rotation)
        INSPECTOR_END()
    };
} // BreadEngine

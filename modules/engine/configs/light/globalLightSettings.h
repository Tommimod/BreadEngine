#pragma once
#include "environmentAmbientParameters.h"
#include "environmentBackgroudParameters.h"
#include "r3d_environment.h"
#include "../baseYamlConfig.h"
#include "r3d_sky.h"
#include "skyboxProceduralParameters.h"
#include "../assets/textureAsset.h"

namespace BreadEngine {
    struct GlobalLightSettings : BaseYamlConfig
    {
        enum class Type : uint8_t
        {
            Procedural = 0,
            Cubemap,
            Custom
        };

        GlobalLightSettings() = default;

        ~GlobalLightSettings() override = default;

        void serializeConfig() override;

        void deserializeConfig(const char *filePath) override;

    private:
        friend class GlobalLightSystem;

        TextureAsset *_skyboxTexture = nullptr;
        R3D_Environment *_nativeEnvironment = nullptr;
        SkyboxProceduralParameters _proceduralSkyboxSettings;
        R3D_ProceduralSky _nativeProceduralSky{};
        std::string _skyboxShaderPath;
        EnvironmentBackgroudParameters _environmentBackgroud{};
        EnvironmentAmbientParameters _environmentAmbient{};
        R3D_Tonemap _tonemap = R3D_TONEMAP_AGX;

        Type _type = Type::Procedural;

        INSPECTOR_BEGIN(GlobalLightSettings)
            INSPECT_FIELD(_type);
            INSPECT_FIELD_COND(_proceduralSkyboxSettings, [](const GlobalLightSettings* s){return s->_type == Type::Procedural;});
            INSPECT_FIELD_COND(_skyboxTexture, [](const GlobalLightSettings* s){return s->_type == Type::Cubemap;});
            INSPECT_FIELD_COND(_skyboxShaderPath, [](const GlobalLightSettings* s){return s->_type == Type::Custom;});
            INSPECT_FIELD(_tonemap);
            INSPECT_FIELD(_environmentBackgroud);
            INSPECT_FIELD(_environmentAmbient);
        INSPECTOR_END()
    };
} // BreadEngine

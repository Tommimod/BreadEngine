#pragma once
#include "environmentAmbientParameters.h"
#include "environmentBackgroudParameters.h"
#include "environmentBloomParameters.h"
#include "environmentColorParameters.h"
#include "environmentDOFParameters.h"
#include "environmentFogParameters.h"
#include "environmentSSAOParameters.h"
#include "environmentSSGIParameters.h"
#include "environmentSSILParameters.h"
#include "environmentSSRParameters.h"
#include "environmentTonemapParameters.h"
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
        SkyboxProceduralParameters _proceduralSkyboxSettings{};
        R3D_ProceduralSky _nativeProceduralSky{};
        std::string _skyboxShaderPath;

        EnvironmentBackgroudParameters _background{};
        EnvironmentAmbientParameters _ambient{};
        EnvironmentSSAOParameters _ssao{};
        EnvironmentSSILParameters _ssil{};
        EnvironmentSSGIParameters _ssgi{};
        EnvironmentSSRParameters _ssr{};
        EnvironmentBloomParameters _bloom{};
        EnvironmentFogParameters _fog{};
        EnvironmentDOFParameters _depthOfField{};
        EnvironmentTonemapParameters _tonemap{};
        EnvironmentColorParameters _finalColor{};

        Type _type = Type::Procedural;

        INSPECTOR_BEGIN(GlobalLightSettings)
            INSPECT_FIELD(_type);
            INSPECT_FIELD_COND(_proceduralSkyboxSettings, [](const GlobalLightSettings* s){return s->_type == Type::Procedural;});
            INSPECT_FIELD_COND(_skyboxTexture, [](const GlobalLightSettings* s){return s->_type == Type::Cubemap;});
            INSPECT_FIELD_COND(_skyboxShaderPath, [](const GlobalLightSettings* s){return s->_type == Type::Custom;});
            INSPECT_FIELD(_background);
            INSPECT_FIELD(_ambient);
            INSPECT_FIELD(_ssao);
            INSPECT_FIELD(_ssil);
            INSPECT_FIELD(_ssgi);
            INSPECT_FIELD(_ssr);
            INSPECT_FIELD(_bloom);
            INSPECT_FIELD(_fog);
            INSPECT_FIELD(_depthOfField);
            INSPECT_FIELD(_tonemap);
            INSPECT_FIELD(_finalColor);
        INSPECTOR_END()
    };
} // BreadEngine

#pragma once
#include "environmentSettings.h"
#include "skyboxCubemapParameters.h"
#include "skyboxProceduralParameters.h"
#include "../baseYamlConfig.h"
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
        SkyboxProceduralParameters _proceduralSkyboxSettings{};
        SkyboxCubemapParameters _cubemapSkyboxSettings{};
        std::string _skyboxShaderPath;

        EnvironmentBackgroudParameters _background{};
        EnvironmentAmbientParameters _ambient{};
        EnvironmentSSAOParameters _ssao{};
        EnvironmentSSRParameters _ssr{};
        EnvironmentBloomParameters _bloom{};
        EnvironmentFogParameters _fog{};
        EnvironmentDOFParameters _depthOfField{};
        EnvironmentTonemapParameters _tonemap{};
        EnvironmentColorParameters _finalColor{};

        Type _type = Type::Procedural;

        [[nodiscard]] EnvironmentSettings environment()
        {
            return EnvironmentSettings{
                .background = _background,
                .ambient = _ambient,
                .ssao = _ssao,
                .ssr = _ssr,
                .bloom = _bloom,
                .fog = _fog,
                .depthOfField = _depthOfField,
                .tonemap = _tonemap,
                .finalColor = _finalColor
            };
        }

        INSPECTOR_BEGIN(GlobalLightSettings)
            INSPECT_FIELD(_type);
            INSPECT_FIELD_COND(_proceduralSkyboxSettings, [](const GlobalLightSettings* s){return s->_type == Type::Procedural;});
            INSPECT_FIELD_COND(_skyboxTexture, [](const GlobalLightSettings* s){return s->_type == Type::Cubemap;});
            INSPECT_FIELD_COND(_cubemapSkyboxSettings, [](const GlobalLightSettings* s){return s->_type == Type::Cubemap;});
            INSPECT_FIELD_COND(_skyboxShaderPath, [](const GlobalLightSettings* s){return s->_type == Type::Custom;});
            INSPECT_FIELD(_background);
            INSPECT_FIELD(_ambient);
            INSPECT_FIELD(_ssao);
            INSPECT_FIELD(_ssr);
            INSPECT_FIELD(_bloom);
            INSPECT_FIELD(_fog);
            INSPECT_FIELD(_depthOfField);
            INSPECT_FIELD(_tonemap);
            INSPECT_FIELD(_finalColor);
        INSPECTOR_END()
    };
} // BreadEngine

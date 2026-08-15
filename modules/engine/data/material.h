#pragma once
#include "inspectorObject.h"
#include "../configs/assets/textureAsset.h"
#include "../rendering/renderTypes.h"

namespace BreadEngine {
    struct Material : InspectorStruct
    {
        Material() = default;

        void setShaderPath(const std::string &path) { _shaderPath = path; }

        void unload();

        /// Resolves the linked texture assets, starting the load of any that aren't resident.
        const MaterialData &getData();

        void setAlbedoTexture(TextureAsset *texture) { _albedoTexture = texture; }
        void setNormalTexture(TextureAsset *texture) { _normalTexture = texture; }
        void setOmrTexture(TextureAsset *texture) { _omrTexture = texture; }
        void setEmissionTexture(TextureAsset *texture) { _emissionTexture = texture; }

    private:
        std::string _shaderPath;
        MaterialData _data{};
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_omrTexture = nullptr;
        TextureAsset *_emissionTexture = nullptr;

        // Edited and serialized, but not yet fed to the renderer. Wiring them up needs a
        // migration first: materials saved so far carry _uvScale {0, 0}, which would collapse
        // every UV the moment it starts being applied.
        Vector2 _uvOffset{};
        Vector2 _uvScale{};
        float _alphaCutoff = 0.01f;

        INSPECTOR_BEGIN(Material)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_omrTexture);
            INSPECT_FIELD(_emissionTexture);
            INSPECT_FIELD(_uvOffset);
            INSPECT_FIELD(_uvScale);
            INSPECT_FIELD(_alphaCutoff);
        INSPECTOR_END()
    };
} // BreadEngine

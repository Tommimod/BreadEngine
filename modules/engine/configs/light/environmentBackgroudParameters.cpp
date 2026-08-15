#include "environmentBackgroudParameters.h"

#include "logger.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentBackgroudParameters)

    void EnvironmentBackgroudParameters::setTexture(TextureAsset *texture)
    {
        if (texture->getTextureType() != TextureAsset::TextureType::Cube)
        {
            Logger::LogError("Texture is not a cube map");
            return;
        }

        clearTexture();
        _skyboxTexture = texture;
        sky = Renderer::get().loadCubemap(texture->getAssetPath());
    }

    void EnvironmentBackgroudParameters::clearTexture()
    {
        _skyboxTexture = nullptr;
        Renderer::get().destroyCubemap(sky);
        sky = {};
    }
} // BreadEngine

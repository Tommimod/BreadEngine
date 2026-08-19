#include "environmentBackgroudParameters.h"

#include "logger.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentBackgroudParameters)

    void EnvironmentBackgroudParameters::setTexture(TextureAsset *texture, const SkyboxCubemapParameters &settings)
    {
        if (texture->getTextureType() != TextureAsset::TextureType::Cube)
        {
            Logger::LogError("Texture is not a cube map");
            return;
        }

        clearTexture();
        sky = Renderer::get().loadCubemap(texture->getAssetPath(), settings);
    }

    void EnvironmentBackgroudParameters::clearTexture()
    {
        Renderer::get().destroyCubemap(sky);
        sky = {};
    }
} // BreadEngine

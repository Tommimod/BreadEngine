#include "environmentBackgroudParameters.h"

#include "logger.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentBackgroudParameters)

    EnvironmentBackgroudParameters &EnvironmentBackgroudParameters::fromNative(const R3D_EnvBackground &nativeData)
    {
        color = nativeData.color;
        energy = nativeData.energy;
        skyBlur = nativeData.skyBlur;
        rotation = nativeData.rotation;
        sky = nativeData.sky;
        return *this;
    }

    R3D_EnvBackground EnvironmentBackgroudParameters::toNative() const
    {
        return R3D_EnvBackground{
            .color = color,
            .energy = energy,
            .skyBlur = skyBlur,
            .sky = sky,
            .rotation = rotation,
        };
    }

    void EnvironmentBackgroudParameters::setTexture(TextureAsset *texture)
    {
        if (texture->getTextureType() != TextureAsset::TextureType::Cube)
        {
            Logger::LogError("Texture is not a cube map");
            return;
        }

        _skyboxTexture = texture;
        sky = _skyboxTexture->getCubemap();
    }

    void EnvironmentBackgroudParameters::clearTexture()
    {
        if (_skyboxTexture != nullptr)
        {
            _skyboxTexture = nullptr;
            sky = R3D_Cubemap{};
        }
        else
        {
            R3D_UnloadCubemap(sky);
            sky = R3D_Cubemap{};
        }
    }
} // BreadEngine

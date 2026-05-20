#include "environmentAmbientParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentAmbientParameters)

    EnvironmentAmbientParameters &EnvironmentAmbientParameters::fromNative(const R3D_EnvAmbient &nativeData)
    {
        color = nativeData.color;
        energy = nativeData.energy;
        map = nativeData.map;
        return *this;
    }

    R3D_EnvAmbient EnvironmentAmbientParameters::toNative() const
    {
        return R3D_EnvAmbient{
            .color = color,
            .energy = energy,
            .map = map
        };
    }

    void EnvironmentAmbientParameters::generateFromCubemap(const R3D_Cubemap &cubemap)
    {
        map = R3D_GenAmbientMap(cubemap, R3D_AMBIENT_ILLUMINATION | R3D_AMBIENT_REFLECTION);
    }

    void EnvironmentAmbientParameters::clear()
    {
        R3D_UnloadAmbientMap(map);
        map = R3D_AmbientMap{};
    }
} // BreadEngine

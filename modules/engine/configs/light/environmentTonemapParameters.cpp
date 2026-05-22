#include "environmentTonemapParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentTonemapParameters)

    EnvironmentTonemapParameters &EnvironmentTonemapParameters::fromNative(const R3D_EnvTonemap &nativeData)
    {
        mode = nativeData.mode;
        exposure = nativeData.exposure;
        white = nativeData.white;
        return *this;
    }

    R3D_EnvTonemap EnvironmentTonemapParameters::toNative() const
    {
        return R3D_EnvTonemap{
            .mode = mode,
            .exposure = exposure,
            .white = white
        };
    }
} // BreadEngine

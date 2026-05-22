#include "environmentDOFParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentDOFParameters)

    EnvironmentDOFParameters &EnvironmentDOFParameters::fromNative(const R3D_EnvDoF &nativeData)
    {
        mode = nativeData.mode;
        focusPoint = nativeData.focusPoint;
        focusScale = nativeData.focusScale;
        nearScale = nativeData.nearScale;
        maxBlurSize = nativeData.maxBlurSize;
        return *this;
    }

    R3D_EnvDoF EnvironmentDOFParameters::toNative() const
    {
        return R3D_EnvDoF{
            .mode = mode,
            .focusPoint = focusPoint,
            .focusScale = focusScale,
            .nearScale = nearScale,
            .maxBlurSize = maxBlurSize
        };
    }
} // BreadEngine

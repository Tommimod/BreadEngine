#include "skyboxProceduralParameters.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(SkyboxProceduralParameters)

    SkyboxProceduralParameters SkyboxProceduralParameters::defaultParameters()
    {
        auto params = SkyboxProceduralParameters();
        params.skyTopColor = Color{98, 116, 140, 255};
        params.skyHorizonColor = Color{165, 167, 171, 255};
        params.skyHorizonCurve = 0.15f;
        params.skyEnergy = 1.0f;
        params.groundBottomColor = Color{51, 43, 34, 255};
        params.groundHorizonColor = Color{165, 167, 171, 255};
        params.groundHorizonCurve = 0.02f;
        params.groundEnergy = 1.0f;
        params.sunDirection = Vector3{-1.0f, -1.0f, -1.0f};
        params.sunColor = Color{255, 255, 255, 255};
        params.sunSize = 1.0f * DEG2RAD;
        params.sunCurve = 0.15f;
        params.sunEnergy = 1.0f;
        params.isCreated = true;
        return params;
    }
} // BreadEngine

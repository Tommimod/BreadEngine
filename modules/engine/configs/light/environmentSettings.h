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

namespace BreadEngine {
    /**
     * The environment as one addressable bundle: references into whoever owns the parameter
     * blocks. Pushing the environment to the renderer and letting the renderer seed it with
     * its own defaults are then the same shape, with no copying and no second declaration of
     * the field list.
     */
    struct EnvironmentSettings
    {
        EnvironmentBackgroudParameters &background;
        EnvironmentAmbientParameters &ambient;
        EnvironmentSSAOParameters &ssao;
        EnvironmentSSILParameters &ssil;
        EnvironmentSSGIParameters &ssgi;
        EnvironmentSSRParameters &ssr;
        EnvironmentBloomParameters &bloom;
        EnvironmentFogParameters &fog;
        EnvironmentDOFParameters &depthOfField;
        EnvironmentTonemapParameters &tonemap;
        EnvironmentColorParameters &finalColor;
    };
} // namespace BreadEngine

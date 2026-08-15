#include "environmentAmbientParameters.h"

#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(EnvironmentAmbientParameters)

    void EnvironmentAmbientParameters::generateFromCubemap(const CubemapHandle cubemap)
    {
        clear();
        map = Renderer::get().createAmbientMap(cubemap);
    }

    void EnvironmentAmbientParameters::clear()
    {
        Renderer::get().destroyAmbientMap(map);
        map = {};
    }
} // BreadEngine

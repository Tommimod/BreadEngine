#include "materialLink.h"

#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MaterialLink)

    MaterialHandle MaterialLink::getHandle() const
    {
        return _materialAsset != nullptr ? _materialAsset->getHandle() : Renderer::defaultMaterial();
    }
} // BreadEngine

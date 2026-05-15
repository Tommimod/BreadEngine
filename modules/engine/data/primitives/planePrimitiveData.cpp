#include "planePrimitiveData.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(PlanePrimitiveData)

    PlanePrimitiveData &PlanePrimitiveData::asQuad()
    {
        _type = MeshPrimitiveType::Quad;
        return *this;
    }
} // BreadEngine

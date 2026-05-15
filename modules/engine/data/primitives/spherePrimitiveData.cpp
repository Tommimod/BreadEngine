#include "spherePrimitiveData.h"

namespace BreadEngine {
    SpherePrimitiveData &SpherePrimitiveData::asHalf()
    {
        _type = MeshPrimitiveType::HalfSphere;
        return *this;
    }

    DEFINE_STATIC_PROPS(SpherePrimitiveData)
} // BreadEngine

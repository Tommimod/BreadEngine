#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct SpherePrimitiveData : MeshPrimitiveData
    {
        float radius = 1.0f;
        int rings = 30;
        int slices = 30;

        SpherePrimitiveData()
        {
            _type = MeshPrimitiveType::Sphere;

            radius = 1.0f;
            rings = 30;
            slices = 30;
        }

        ~SpherePrimitiveData() override = default;

        SpherePrimitiveData &asHalf();

    private:
        INSPECTOR_BEGIN(SpherePrimitiveData)
            INSPECT_FIELD(radius);
            INSPECT_FIELD(rings);
            INSPECT_FIELD(slices);
        INSPECTOR_END()
    };
} // BreadEngine

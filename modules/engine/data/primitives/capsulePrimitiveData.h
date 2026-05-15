#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct CapsulePrimitiveData : MeshPrimitiveData
    {
        float height = 1.0f;
        float radius = 1.0f;
        int rings = 10;
        int slices = 30;

        CapsulePrimitiveData()
        {
            _type = MeshPrimitiveType::Capsule;

            height = 1.0f;
            radius = 1.0f;
            rings = 10;
            slices = 30;
        }

        ~CapsulePrimitiveData() override = default;

    private:
        INSPECTOR_BEGIN(CapsulePrimitiveData)
            INSPECT_FIELD(height);
            INSPECT_FIELD(radius);
            INSPECT_FIELD(rings);
            INSPECT_FIELD(slices);
        INSPECTOR_END()
    };
} // BreadEngine

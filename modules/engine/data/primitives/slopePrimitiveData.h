#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct SlopePrimitiveData : MeshPrimitiveData
    {
        float width = 1.0f;
        float height = 1.0f;
        float length = 1.0f;
        Vector3 normal = Vector3{1, 1, 0};

        SlopePrimitiveData()
        {
            _type = MeshPrimitiveType::Slope;

            width = 1.0f;
            height = 1.0f;
            length = 1.0f;
            normal = Vector3{1, 1, 0};
        }

        ~SlopePrimitiveData() override = default;

    private:
        INSPECTOR_BEGIN(SlopePrimitiveData)
            INSPECT_FIELD(width);
            INSPECT_FIELD(height);
            INSPECT_FIELD(length);
            INSPECT_FIELD(normal);
        INSPECTOR_END()
    };
} // BreadEngine

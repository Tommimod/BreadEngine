#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct CubePrimitiveData : MeshPrimitiveData
    {
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;

        CubePrimitiveData()
        {
            _type = MeshPrimitiveType::Cube;

            width = 1.0f;
            height = 1.0f;
            depth = 1.0f;
        }

        ~CubePrimitiveData() override = default;

    private:
        INSPECTOR_BEGIN(CubePrimitiveData)
            INSPECT_FIELD(width);
            INSPECT_FIELD(height);
            INSPECT_FIELD(depth);
        INSPECTOR_END()
    };
} // BreadEngine

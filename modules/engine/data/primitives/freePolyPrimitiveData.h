#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct FreePolyPrimitiveData : MeshPrimitiveData
    {
        int sides = 4;
        float size = 1.0f;

        FreePolyPrimitiveData()
        {
            _type = MeshPrimitiveType::FreePoly;

            sides = 4;
            size = 1.0f;
        }

        ~FreePolyPrimitiveData() override = default;

        FreePolyPrimitiveData &asHalf();

    private:
        INSPECTOR_BEGIN(FreePolyPrimitiveData)
            INSPECT_FIELD(size);
            INSPECT_FIELD(sides);
        INSPECTOR_END()
    };
} // BreadEngine

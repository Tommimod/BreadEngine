#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct TorusPrimitiveData : MeshPrimitiveData
    {
        float radius = 1.0f;
        float size = 0.5;
        int radiusSegments = 10;
        int sides = 10;

        TorusPrimitiveData()
        {
            _type = MeshPrimitiveType::Torus;

            radius = 1.0f;
            size = .5f;
            radiusSegments = 10;
            sides = 10;
        }

        ~TorusPrimitiveData() override = default;

        TorusPrimitiveData &asHalf();

    private:
        INSPECTOR_BEGIN(TorusPrimitiveData)
            INSPECT_FIELD(radius);
            INSPECT_FIELD(size);
            INSPECT_FIELD(radiusSegments);
            INSPECT_FIELD(sides);
        INSPECTOR_END()
    };
} // BreadEngine

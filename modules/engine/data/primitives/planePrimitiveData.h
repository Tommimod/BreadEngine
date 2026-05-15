#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct PlanePrimitiveData : MeshPrimitiveData
    {
        float width = 1.0f;
        float height = 1.0f;
        int resX = 1;
        int resZ = 1;

        PlanePrimitiveData()
        {
            _type = MeshPrimitiveType::Plane;

            width = 1.0f;
            height = 1.0f;
            resX = 1;
            resZ = 1;
        }

        PlanePrimitiveData &asQuad();

        ~PlanePrimitiveData() override = default;

    private:
        INSPECTOR_BEGIN(PlanePrimitiveData)
            INSPECT_FIELD(width);
            INSPECT_FIELD(height);
            INSPECT_FIELD(resX);
            INSPECT_FIELD(resZ);
        INSPECTOR_END()
    };
} // BreadEngine

#pragma once
#include "meshPrimitiveData.h"

namespace BreadEngine {
    struct CylinderPrimitiveData : MeshPrimitiveData
    {
        float bottomRadius = .5f;
        float topRadius = .5f;
        float height = 1;
        int slices = 30;
        int stacks = 1;
        bool bottomCap = true;
        bool topCap = true;

        CylinderPrimitiveData()
        {
            _type = MeshPrimitiveType::Cylinder;

            bottomRadius = .5f;
            topRadius = .5f;
            height = 1;
            slices = 30;
            stacks = 1;
            bottomCap = true;
            topCap = true;
        }

        ~CylinderPrimitiveData() override = default;

    private:
        INSPECTOR_BEGIN(CylinderPrimitiveData)
            INSPECT_FIELD(bottomRadius);
            INSPECT_FIELD(topRadius);
            INSPECT_FIELD(height);
            INSPECT_FIELD(slices);
            INSPECT_FIELD(stacks);
            INSPECT_FIELD(bottomCap);
            INSPECT_FIELD(topCap);
        INSPECTOR_END()
    };
} // BreadEngine

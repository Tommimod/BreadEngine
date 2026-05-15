#pragma once
#include "inspectorObject.h"
#include "meshPrimitiveType.h"

namespace BreadEngine {
    struct MeshPrimitiveData : InspectorStruct
    {
        MeshPrimitiveType getMeshType() const { return _type; }

    protected:
        MeshPrimitiveType _type = MeshPrimitiveType::None;

    private:
        INSPECTOR_BEGIN(MeshPrimitiveData)
            INSPECT_FIELD(_type)
        INSPECTOR_END()
    };
} // BreadEngine

#pragma once
#include "raygizmo.h"
#include "transform.h"

namespace BreadEditor {
    class GizmoSystem
    {
    public:
        GizmoSystem() = default;

        ~GizmoSystem() = default;

        void Draw(const BreadEngine::Transform &nodeTransform);

    private:
        GizmoFlags _mode = GIZMO_TRANSLATE;
        ::Transform _transform;
    };
} // BreadEditor

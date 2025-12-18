#pragma once
#include "raygizmo.h"
#include "transform.h"

namespace BreadEditor {
    class GizmoSystem
    {
    public:
        GizmoSystem() = default;

        ~GizmoSystem() = default;

        void setMode(const GizmoFlags mode) { _mode = mode; }

        void recalculateGizmo(BreadEngine::Transform &nodeTransform);

        void render();

    private:
        GizmoFlags _mode = GIZMO_TRANSLATE;
        ::Transform _transform{};
        BreadEngine::Transform *_nodeTransform{};
    };
} // BreadEditor

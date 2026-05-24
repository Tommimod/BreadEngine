#pragma once
#include "raygizmo.h"
#include "transform.h"
#include "../windows/viewportWindow.h"

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
        ::Transform _prevTransform{};
        BreadEngine::Transform *_nodeTransform{};
        ViewportWindow *_viewportWindow = nullptr;

        bool isPrevTransformInitialized() const;
    };
} // BreadEditor

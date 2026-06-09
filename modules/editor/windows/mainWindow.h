#pragma once
#include "assetsWindow.h"
#include "consoleWindow.h"
#include "../systems/gizmoSystem.h"
#include "propertyInspectorWindow.h"
#include "nodeTreeWindow.h"
#include "../uitoolkit/uiToolbar.h"
#include "raylib.h"
#include "viewportWindow.h"
#include "systems/mainToolbarSystem.h"
#include "uitoolkit/uiSide.h"
using namespace BreadEngine;

namespace BreadEditor {
    class MainWindow final : public UiElement
    {
    public:
        static Vector2 getWindowSize();

        MainWindow();

        ~MainWindow() override;

        void initialize();

        [[nodiscard]] vector<std::string> &getWindowsOptions();

        [[nodiscard]] UiToolbar &getToolbar() const;

        [[nodiscard]] NodeTreeWindow &getNodeTree() const;

        [[nodiscard]] PropertyInspectorWindow &getPropertyInspector() const;

        [[nodiscard]] GizmoSystem &getGizmoSystem();

        [[nodiscard]] AssetsWindow &getAssetsWindow() const;

        [[nodiscard]] ConsoleWindow &getConsoleWindow() const;

        [[nodiscard]] ViewportWindow &getViewportWindow() const;

        void render3D(float deltaTime);

    protected:
        void awake() override;

        bool tryDeleteSelf() override;

    private:
        friend class Editor;
        unique_ptr<UiSide> _leftSide;
        unique_ptr<UiSide> _rightSide;
        unique_ptr<UiSide> _bottomSide;
        unique_ptr<UiSide> _centerSide;

        GizmoSystem _gizmoSystem;
        MainToolbarSystem _mainToolbarSystem;
        vector<std::string> _windowsOptions{};

        [[nodiscard]] UiElement *findUiElementById(const std::string &id) const;

        [[nodiscard]] UiToolbar &getToolbar();

        void resize();

        void processRaycast(Vector2 ray) const;

        static void fillChildListByDepth(const UiElement *element, const Vector2 &ray, std::array<std::vector<UiElement *>, maxChildsDepth> &result);
    };
} // namespace BreadEditor

#pragma once
#include "windows/mainWindow.h"
#include <string>
#include <memory>

#include "component/camera.h"
#include "configs/infrastructure/configsProvider.h"
#include "models/editorModel.h"
#include "rendering/gridRenderer.h"
#include "systems/cameraSystem.h"
#include "utils/filesWatcher.h"
using namespace BreadEngine;
#if !defined(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT)
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT        24
#endif
namespace BreadEditor {
    class Editor
    {
    public:
        Editor();

        ~Editor() = default;

        MainWindow mainWindow{};

        static Editor &getInstance();

        bool initialize();

        void shutdown();

        void callLoop();

        void update(float deltaTime);

        void render2D(float deltaTime);

        void renderOverlay(const Camera3D &camera);

        bool createProject(const std::string &name, const std::string &path);

        bool openProject(const std::string &path);

        void closeProject();

        [[nodiscard]] bool compileGame();

        bool runGame();

        void stopGame();

        void pauseGame();

        void resumeGame();

        Camera3D &getCamera()
        {
            return _camera;
        }

        [[nodiscard]] bool isCameraRendered() const { return _isCameraRendered; }

        [[nodiscard]] bool isPlayMode() const { return _isPlayMode; }

        [[nodiscard]] bool isEditorMode() const { return !_isPlayMode; }

        [[nodiscard]] bool isPaused() const { return _isPaused; }

        [[nodiscard]] EditorModel &getEditorModel() { return _editorModel; }

        [[nodiscard]] ConfigsProvider &getConfigsProvider() { return _configsProvider; }

        [[nodiscard]] bool isFrameEnded() const { return _isFrameEnded; }

    private:
        static std::unique_ptr<Editor> _instance;

        bool _isPlayMode = false;
        bool _isPaused = false;
        bool _isFrameEnded = false;
        bool _initialized = false;
        bool _isCameraRendered = false;
        UiElement &_uiRoot;
        Camera3D _camera{};

        EditorModel _editorModel;
        ConfigsProvider _configsProvider;
        CameraSystem _cameraSystem;
        GridRenderer _gridRenderer;

        static void processInput();

        void setupDefaultCamera();

        static BreadEngine::Camera *getGameCamera();
    };
} // namespace BreadEditor

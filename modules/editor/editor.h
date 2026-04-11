#pragma once
#include "windows/mainWindow.h"
#include <string>
#include <memory>
#include "editorStyle.h"
#include "configs/infrastructure/configsProvider.h"
#include "models/editorModel.h"
#include "systems/cameraSystem.h"
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

        void update(float deltaTime);

        void render2D(RenderTexture2D &renderTexture, float deltaTime);

        void render3D(float deltaTime);

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

        [[nodiscard]] bool isPlayMode() const { return _isPlayMode; }

        [[nodiscard]] bool isEditorMode() const { return !_isPlayMode; }

        [[nodiscard]] bool isPaused() const { return _isPaused; }

        [[nodiscard]] EditorModel &getEditorModel() { return _editorModel; }

        [[nodiscard]] RenderTexture2D *getViewportRenderTexture() const { return _viewportRenderTexture; }

        [[nodiscard]] ConfigsProvider &getConfigsProvider() { return _configsProvider; }

        [[nodiscard]] bool isFrameEnded() const { return _isFrameEnded; }

    private:
        static std::unique_ptr<Editor> _instance;

        bool _isPlayMode = false;
        bool _isPaused = false;
        bool _isFrameEnded = false;
        bool _initialized = false;
        RenderTexture2D *_viewportRenderTexture = nullptr;
        UiElement &_uiRoot;
        Camera3D _camera;

        EditorModel _editorModel;
        ConfigsProvider _configsProvider;
        CameraSystem _cameraSystem;

        static void processInput();
        void setupDefaultCamera();
    };
} // namespace BreadEditor

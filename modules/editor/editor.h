#pragma once
#include "windows/mainWindow.h"
#include <string>
#include <memory>
#include "engine.h"
#include "configs/infrastructure/configsProvider.h"
#include "models/editorModel.h"
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

        void setEngine(Engine &engine);

        bool initialize();

        void shutdown();

        void update(float deltaTime);

        void render2D(RenderTexture2D &renderTexture, float deltaTime);

        void render3D(float deltaTime);

        bool createProject(const std::string &name, const std::string &path);

        bool openProject(const std::string &path);

        void closeProject();

        [[nodiscard]] bool compileGame() const;

        bool runGame();

        void stopGame();

        [[nodiscard]] bool isProjectOpen() const { return !_currentProjectPath.empty(); }

        [[nodiscard]] const std::string &getCurrentProjectPath() const { return _currentProjectPath; }

        [[nodiscard]] EditorModel &getEditorModel() { return _editorModel; }

        [[nodiscard]] RenderTexture2D *getViewportRenderTexture() const { return _viewportRenderTexture; }

        [[nodiscard]] ConfigsProvider &getConfigsProvider() { return _configsProvider; }

    private:
        static std::unique_ptr<Editor> _instance;

        bool _initialized = false;
        std::string _currentProjectPath;
        RenderTexture2D *_viewportRenderTexture = nullptr;
        Engine *_engine = nullptr;
        UiElement &_uiRoot;

        EditorModel _editorModel;
        ConfigsProvider _configsProvider;
    };
} // namespace BreadEditor

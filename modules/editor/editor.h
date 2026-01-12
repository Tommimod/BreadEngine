#pragma once
#include "windows/mainWindow/mainWindow.h"
#include <string>
#include "engine.h"
#include "models/editorModel.h"
using namespace BreadEngine;

namespace BreadEditor {
    class Editor
    {
    public:
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

        [[nodiscard]] RenderTexture2D* getViewportRenderTexture() const { return _viewportRenderTexture; }

    private:
        Editor();

        ~Editor() = default;

        bool _initialized = false;
        std::string _currentProjectPath;
        Engine *_engine = nullptr;
        EditorModel _editorModel;
        UiElement &_uiRoot;
        RenderTexture2D* _viewportRenderTexture;
    };
} // namespace BreadEditor

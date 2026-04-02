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

        [[nodiscard]] bool isPlayMode() const { return _isPlayMode; }

        [[nodiscard]] bool isEditorMode() const { return !_isPlayMode; }

        [[nodiscard]] EditorModel &getEditorModel() { return _editorModel; }

        [[nodiscard]] RenderTexture2D *getViewportRenderTexture() const { return _viewportRenderTexture; }

        [[nodiscard]] ConfigsProvider &getConfigsProvider() { return _configsProvider; }

        void setFontSize(EditorStyle::FontSize size) const;

        void setFontSize(int size) const;

        [[nodiscard]] bool isFrameEnded() const { return _isFrameEnded; }

    private:
        static std::unique_ptr<Editor> _instance;

        bool _isPlayMode = false;
        bool _isFrameEnded = false;
        bool _initialized = false;
        RenderTexture2D *_viewportRenderTexture = nullptr;
        UiElement &_uiRoot;
        Font _fontSmall{};
        Font _fontSmallMedium{};
        Font _fontMedium{};
        Font _fontMediumLarge{};
        Font _fontLargeSmall{};
        Font _fontLarge{};

        EditorModel _editorModel;
        ConfigsProvider _configsProvider;
        CameraSystem _cameraSystem;

        void loadFont();

        static void processInput();
    };
} // namespace BreadEditor

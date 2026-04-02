#include "../engine/engine.h"
#include "editor.h"
#include <fstream>
#include <models/reservedFileNames.h>

#include "systems/cursorSystem.h"
#include "node.h"
#include "systems/commands/commandsHandler.h"
#include "systems/commands/mainToolbarCommands/reopenLastProjectCommand.h"
#include "systems/commands/mainToolbarCommands/saveProjectCommand.h"
#include "validators/mandatoryEditorFilesValidator.h"

namespace BreadEditor {
    std::unique_ptr<Editor> Editor::_instance = std::make_unique<Editor>();

    Editor::Editor() : _uiRoot(mainWindow)
    {
    }

    Editor &Editor::getInstance()
    {
        return *_instance;
    }

    bool Editor::initialize()
    {
        if (_initialized) return false;

        _initialized = MandatoryEditorFilesValidator::validate();
        CommandsHandler::execute(std::make_unique<ReopenLastProjectCommand>());
        Engine::getInstance().getAssetsConfig().deserializeConfig(_configsProvider.getEditorPrefsConfig()->LastProjectPath.c_str());

        GuiLoadStyleDefault();
        loadFont();
        SetTraceLogLevel(LOG_ALL);

        mainWindow.initialize();
        mainWindow.updateInternal(0);
        mainWindow.drawInternal(0);
        const auto filePath = _configsProvider.getEditorPrefsConfig()->LastOpenedNodePath;
        Node::deserialize(filePath);

        runGame();
        return _initialized;
    }

    void Editor::shutdown()
    {
        if (!_initialized) return;

        closeProject();
        _initialized = false;
    }

    void Editor::update(const float deltaTime)
    {
        if (!_initialized) return;

        processInput();
        _isFrameEnded = false;
        mainWindow.updateInternal(deltaTime);
        _cameraSystem.update(deltaTime);
        CursorSystem::draw();
    }

    void Editor::render2D(RenderTexture2D &renderTexture, const float deltaTime)
    {
        if (!_initialized) return;

        _viewportRenderTexture = &renderTexture;
        mainWindow.drawInternal(deltaTime);
        _isFrameEnded = true;
    }

    void Editor::render3D(const float deltaTime)
    {
        if (!_initialized) return;

        mainWindow.render3D(deltaTime);
    }

    bool Editor::createProject(const std::string &name, const std::string &path)
    {
        return true;
    }

    bool Editor::openProject(const std::string &path)
    {
        return true;
    }

    void Editor::closeProject()
    {
        UnloadFont(_fontSmall);
        UnloadFont(_fontSmallMedium);
        UnloadFont(_fontMedium);
        UnloadFont(_fontMediumLarge);
        UnloadFont(_fontLargeSmall);
        UnloadFont(_fontLarge);
    }

    bool Editor::compileGame()
    {
        Logger::LogInfo("Compiling game...");
        std::string projectPath = getEditorModel().getProjectPath();
        
        // If path points to cmake-build-debug/bin/, go up 2 levels to get build root
        std::string buildDir = projectPath;
        if (projectPath.find("cmake-build-debug") != std::string::npos) {
            // Already in build directory, use it directly
            // Remove trailing bin/ if present and use cmake-build-debug as build root
            size_t pos = projectPath.rfind("bin");
            if (pos != std::string::npos) {
                buildDir = projectPath.substr(0, pos);
            }
        }
        
        // Ensure path ends with cmake-build-debug
        if (buildDir.find("cmake-build-debug") == std::string::npos) {
            buildDir = projectPath + "../../cmake-build-debug";
        }

        std::string buildCommand = "cmake --build " + buildDir + " --target ExampleGameLib -j 14";
        Logger::LogInfo(buildCommand.c_str());
        return system(buildCommand.c_str()) == 0;
    }

    bool Editor::runGame()
    {
        if (!compileGame()) return false;

        std::string projectPath = getEditorModel().getProjectPath();
        std::string buildDir = projectPath;
        if (projectPath.find("cmake-build-debug") != std::string::npos) {
            size_t pos = projectPath.rfind("bin");
            if (pos != std::string::npos) {
                buildDir = projectPath.substr(0, pos);
            }
        }
        if (buildDir.find("cmake-build-debug") == std::string::npos) {
            buildDir = projectPath + "../../cmake-build-debug";
        }

#ifdef _WIN32
        const std::string gamePath = buildDir + "bin\\libgame.dll";
#elif defined(__APPLE__)
        std::string gamePath = buildDir + "bin\\libgame.dylib";
#else
        std::string gamePath = buildDir + "bin\\libgame.so";
#endif
        Logger::LogInfo(gamePath);
        Engine::getInstance().loadGameModule(gamePath.c_str());
        return true;
    }

    void Editor::stopGame()
    {
        Engine::getInstance().unloadGameModule();
    }

    void Editor::setFontSize(const EditorStyle::FontSize size) const
    {
        switch (size)
        {
            case EditorStyle::FontSize::Small: GuiSetFont(_fontSmall);
                break;
            case EditorStyle::FontSize::SmallMedium: GuiSetFont(_fontSmallMedium);
                break;
            case EditorStyle::FontSize::Medium: GuiSetFont(_fontMedium);
                break;
            case EditorStyle::FontSize::MediumLarge: GuiSetFont(_fontMediumLarge);
                break;
            case EditorStyle::FontSize::LargeSmall: GuiSetFont(_fontLargeSmall);
                break;
            case EditorStyle::FontSize::Large: GuiSetFont(_fontLarge);
                break;
            default: throw new std::runtime_error("Invalid font size");
        }
    }

    void Editor::setFontSize(const int size) const
    {
        setFontSize(static_cast<EditorStyle::FontSize>(size));
    }

    void Editor::loadFont()
    {
        const auto path = TextFormat("%s/%s", GetApplicationDirectory(), R"(assets\editor\fonts\Roboto.ttf)");
        _fontSmall = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::Small), nullptr, 0);
        _fontSmallMedium = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::SmallMedium), nullptr, 0);
        _fontMedium = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::Medium), nullptr, 0);
        _fontMediumLarge = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::MediumLarge), nullptr, 0);
        _fontLargeSmall = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::LargeSmall), nullptr, 0);
        _fontLarge = LoadFontEx(path, static_cast<int>(EditorStyle::FontSize::Large), nullptr, 0);
        GuiSetFont(_fontMedium);
    }

    void Editor::processInput()
    {
        const auto isCtrlPressed = IsKeyDown(KEY_LEFT_CONTROL);
        if (const auto isZPressedOnce = IsKeyPressed(KEY_Z); isCtrlPressed && isZPressedOnce)
        {
            CommandsHandler::undo();
        }
        else if (const auto isSPressedOnce = IsKeyPressed(KEY_S); isCtrlPressed && isSPressedOnce)
        {
            CommandsHandler::execute(std::make_unique<SaveProjectCommand>());
        }
    }
} // namespace BreadEditor

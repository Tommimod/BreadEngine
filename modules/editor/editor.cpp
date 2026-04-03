#include "../engine/engine.h"
#include "editor.h"
#include <fstream>

#include "systems/cursorSystem.h"
#include "node.h"
#include "systems/commands/commandsHandler.h"
#include "systems/commands/mainToolbarCommands/reopenLastProjectCommand.h"
#include "systems/commands/mainToolbarCommands/saveProjectCommand.h"
#include "tracy/Tracy.hpp"
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
        ZoneScoped;
        if (_initialized) return false;

        _initialized = MandatoryEditorFilesValidator::validate();
        CommandsHandler::execute(std::make_unique<ReopenLastProjectCommand>());
        Engine::getInstance().getAssetsConfig().deserializeConfig(_configsProvider.getEditorPrefsConfig()->LastProjectPath.c_str());

        GuiLoadStyleDefault();
        EditorStyle::loadFont();
        SetTraceLogLevel(LOG_ALL);

        mainWindow.initialize();
        mainWindow.updateInternal(0);
        mainWindow.drawInternal(0);
        const auto filePath = _configsProvider.getEditorPrefsConfig()->LastOpenedNodePath;
        Node::deserialize(filePath);

        return _initialized;
    }

    void Editor::shutdown()
    {
        ZoneScoped;
        if (!_initialized) return;

        closeProject();
        _initialized = false;
    }

    void Editor::update(const float deltaTime)
    {
        ZoneScoped;
        if (!_initialized) return;

        processInput();
        _isFrameEnded = false;
        mainWindow.updateInternal(deltaTime);
        _cameraSystem.update(deltaTime);
        CursorSystem::draw();
    }

    void Editor::render2D(RenderTexture2D &renderTexture, const float deltaTime)
    {
        ZoneScoped;
        if (!_initialized) return;

        _viewportRenderTexture = &renderTexture;
        mainWindow.drawInternal(deltaTime);
        _isFrameEnded = true;
    }

    void Editor::render3D(const float deltaTime)
    {
        ZoneScoped;
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
        ZoneScoped;
        UnloadFont(EditorStyle::_fontSmall);
        UnloadFont(EditorStyle::_fontSmallMedium);
        UnloadFont(EditorStyle::_fontMedium);
        UnloadFont(EditorStyle::_fontMediumLarge);
        UnloadFont(EditorStyle::_fontLargeSmall);
        UnloadFont(EditorStyle::_fontLarge);
    }

    bool Editor::compileGame()
    {
        ZoneScoped;
        Logger::LogInfo("Compiling game...");
        std::string projectPath = getEditorModel().getProjectPath();

        // If path points to cmake-build-debug/bin/, go up 2 levels to get build root
        std::string buildDir = projectPath;
        if (projectPath.find("cmake-build-debug") != std::string::npos)
        {
            // Already in build directory, use it directly
            // Remove trailing bin/ if present and use cmake-build-debug as build root
            size_t pos = projectPath.rfind("bin");
            if (pos != std::string::npos)
            {
                buildDir = projectPath.substr(0, pos);
            }
        }

        // Ensure path ends with cmake-build-debug
        if (buildDir.find("cmake-build-debug") == std::string::npos)
        {
            buildDir = projectPath + "../../cmake-build-debug";
        }

        std::string buildCommand = "cmake --build " + buildDir + " --target ExampleGameLib -j 14";
        Logger::LogInfo(buildCommand.c_str());
        return system(buildCommand.c_str()) == 0;
    }

    bool Editor::runGame()
    {
        ZoneScoped;
        if (!compileGame()) return false;

        std::string projectPath = getEditorModel().getProjectPath();
        std::string buildDir = projectPath;
        if (projectPath.find("cmake-build-debug") != std::string::npos)
        {
            size_t pos = projectPath.rfind("bin");
            if (pos != std::string::npos)
            {
                buildDir = projectPath.substr(0, pos);
            }
        }
        if (buildDir.find("cmake-build-debug") == std::string::npos)
        {
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
        ZoneScoped;
        Engine::getInstance().unloadGameModule();
    }

    void Editor::processInput()
    {
        ZoneScoped;
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

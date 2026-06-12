#include "../engine/engine.h"
#include "editor.h"
#include <fstream>

#include "systems/cursorSystem.h"
#include "node.h"
#include <r3d.h>
#include "commands/commandsHandler.h"
#include "commands/mainToolbarCommands/file/reopenLastProjectCommand.h"
#include "commands/mainToolbarCommands/file/saveProjectCommand.h"
#include "systems/core/filterOption.h"
#include "tracy/Tracy.hpp"
#include "validators/mandatoryEditorFilesValidator.h"
#include "component/cameraDirector.h"
#include "utils/assetsConfigUpdater.h"

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

        FilesWatcher::start(Engine::getProjectPath().c_str());
        AssetsConfigUpdater::subscribe();
        _initialized = MandatoryEditorFilesValidator::validate();
        CommandsHandler::execute(std::make_unique<ReopenLastProjectCommand>());

        GuiLoadStyleDefault();
        EditorStyle::loadFont();
        // EditorStyle::loadStyle(_configsProvider.getEditorPrefsConfig()->EditorThemeName); //TODO Setup editor theme
        SetTraceLogLevel(LOG_ALL);
        setupDefaultCamera();

        mainWindow.initialize();
        mainWindow.updateInternal(0);
        mainWindow.drawInternal(0);

        Engine::getInstance().initializeSystems();
        return _initialized;
    }

    void Editor::shutdown()
    {
        ZoneScoped;
        if (!_initialized) return;

        AssetsConfigUpdater::unsubscribe();
        FilesWatcher::stop();
        closeProject();
        _initialized = false;
    }

    void Editor::callLoop(RenderTexture2D &renderTexture)
    {
        const Engine &engine = Engine::getInstance();
        const auto &viewportWindow = mainWindow.getViewportWindow();
        const auto nextWidth = static_cast<int>(viewportWindow.getViewportSize().width);
        const auto nextHeight = static_cast<int>(viewportWindow.getViewportSize().height);
        if (nextWidth != renderTexture.texture.width || nextHeight != renderTexture.texture.height)
        {
            UnloadRenderTexture(renderTexture);
            renderTexture = LoadRenderTexture(nextWidth, nextHeight);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        const auto viewportMode = viewportWindow.getMode();
        _isCameraRendered = true;
        Camera3D cameraForViewport;
        if (viewportMode == ViewportWindow::Scene)
        {
            cameraForViewport = getCamera();
            R3D_BeginEx(renderTexture, cameraForViewport);
        }
        else
        {
            const auto gameCamera = getGameCamera();
            _isCameraRendered = gameCamera != nullptr;
            cameraForViewport = _isCameraRendered ? gameCamera->getNativeCamera() : getCamera();
            R3D_BeginEx(renderTexture, cameraForViewport);
        }

        const auto deltaTime = isPaused() ? 0 : Engine::getDeltaTime();
        engine.update(deltaTime);
        update(deltaTime);

        if (_isCameraRendered)
        {
            engine.onFrameStart(deltaTime);
            engine.onFrameEnd(deltaTime);
        }
        R3D_End();

        BeginTextureMode(renderTexture); // drawing 3D editor to viewport
        BeginMode3D(cameraForViewport);
        DrawGrid(1000, 1.0f);
        if (_isCameraRendered)
        {
            render3D(deltaTime);
        }
        EndMode3D();
        EndTextureMode(); // end 3D editor of viewport

        if (!_isCameraRendered)
        {
            BeginTextureMode(renderTexture);
            ClearBackground(BLACK);
            EndTextureMode();
        }

        render2D(renderTexture, deltaTime); // drawing editor UI
        EndDrawing();
    }

    void Editor::update(const float deltaTime)
    {
        ZoneScoped;
        if (!_initialized) return;

        processInput();
        CommandsHandler::update();
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
        const auto &projectPath = getEditorModel().getProjectPath();

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

        const auto buildCommand = "cmake --build " + buildDir + " --target ExampleGameLib -j 14 -v";
        Logger::LogInfo(buildCommand);
        return system(buildCommand.c_str()) == 0;
    }

    bool Editor::runGame()
    {
        ZoneScoped;
        if (!compileGame()) return false;

        const auto &projectPath = getEditorModel().getProjectPath();
        auto buildDir = projectPath;
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
        const auto gamePath = buildDir + "bin\\libgame.dll";
#elif defined(__APPLE__)
        const auto gamePath = buildDir + "bin\\libgame.dylib";
#else
        const auto gamePath = buildDir + "bin\\libgame.so";
#endif
        Logger::LogInfo(gamePath);
        Engine::getInstance().loadGameModule(gamePath.c_str());
        _isPlayMode = true;
        return true;
    }

    void Editor::stopGame()
    {
        ZoneScoped;
        Engine::getInstance().unloadGameModule();
        _isPlayMode = false;
    }

    void Editor::pauseGame()
    {
        _isPaused = true;
    }

    void Editor::resumeGame()
    {
        _isPaused = false;
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

    void Editor::setupDefaultCamera()
    {
        _camera.position = {0.0f, 10.0f, 10.0f};
        _camera.target = {0.0f, 0.0f, 0.0f};
        _camera.up = {0.0f, 1.0f, 0.0f};
        _camera.fovy = 45.0f;
        _camera.projection = CAMERA_PERSPECTIVE;
    }

    BreadEngine::Camera *Editor::getGameCamera()
    {
        for (auto &allNodes = NodeProvider::getAllNodes(); const auto node: allNodes)
        {
            if (node->has<CameraDirector>())
            {
                return node->get<CameraDirector>().getActiveCamera();
            }
        }
        return nullptr;
    }
} // namespace BreadEditor

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
        // Create project directory structure
        // Generate basic game template
        _currentProjectPath = path + "/" + name;
        return true;
    }

    bool Editor::openProject(const std::string &path)
    {
        _currentProjectPath = path;
        return true;
    }

    void Editor::closeProject()
    {
        _currentProjectPath.clear();
        UnloadFont(_fontSmall);
        UnloadFont(_fontSmallMedium);
        UnloadFont(_fontMedium);
        UnloadFont(_fontMediumLarge);
        UnloadFont(_fontLargeSmall);
        UnloadFont(_fontLarge);
    }

    bool Editor::compileGame() const
    {
        if (!isProjectOpen()) return false;

        std::string buildCommand = "cd " + _currentProjectPath + " && make";
        return system(buildCommand.c_str()) == 0;
    }

    bool Editor::runGame()
    {
        if (!compileGame()) return false;

        std::string gamePath = _currentProjectPath + "/build/libgame.dylib";
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

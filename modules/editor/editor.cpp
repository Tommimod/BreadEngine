#include "../engine/engine.h"
#include "editor.h"

#include "systems/cursorSystem.h"
#include "node.h"
#include "systems/commands/commandsHandler.h"
#include "systems/commands/mainToolbarCommands/reopenLastProjectCommand.h"
#include "validators/mandatoryEditorFilesValidator.h"

namespace BreadEditor {
    Editor &Editor::getInstance()
    {
        static Editor instance;
        return instance;
    }

    void Editor::setEngine(Engine &engine)
    {
        _engine = &engine;
    }

    bool Editor::initialize()
    {
        if (_initialized) return false;

        _initialized = MandatoryEditorFilesValidator::validate();
        CommandsHandler::execute(std::make_unique<ReopenLastProjectCommand>());
        Engine::getInstance().getFileSystem().initialize(_configsProvider.getEditorPrefsConfig()->LastProjectPath.c_str());

        GuiLoadStyleDefault();
        SetTraceLogLevel(LOG_ALL);

        mainWindow.initialize();
        mainWindow.updateInternal(0);
        mainWindow.drawInternal(0);
        const auto rootNode = &Engine::getRootNode();
        auto &testNode = Engine::nodePool.get().setup("1", *rootNode);
        auto &testNode2 = Engine::nodePool.get().setup("1.1", testNode);
        auto &testNode25 = Engine::nodePool.get().setup("1.2", testNode);
        auto &testNode444 = Engine::nodePool.get().setup("1.1.1", testNode2);
        auto &testNode3 = Engine::nodePool.get().setup("2", *rootNode);
        Engine::nodePool.get().setup("3", *rootNode);
        Engine::nodePool.get().setup("4", *rootNode);
        Engine::nodePool.get().setup("5", *rootNode);
        Engine::nodePool.get().setup("6", *rootNode);
        Engine::nodePool.get().setup("7", *rootNode);
        Engine::nodePool.get().setup("8", *rootNode);
        Engine::nodePool.get().setup("9", *rootNode);
        Engine::nodePool.get().setup("10", *rootNode);
        Engine::nodePool.get().setup("11", *rootNode);
        Engine::nodePool.get().setup("12", *rootNode);
        Engine::nodePool.get().setup("13", *rootNode);
        Engine::nodePool.get().setup("14", *rootNode);
        Engine::nodePool.get().setup("15", *rootNode);
        Engine::nodePool.get().setup("16", *rootNode);
        Engine::nodePool.get().setup("17", *rootNode);
        Engine::nodePool.get().setup("18", *rootNode);
        Engine::nodePool.get().setup("19", *rootNode);
        Engine::nodePool.get().setup("20", *rootNode);
        Engine::nodePool.get().setup("21", *rootNode);
        Engine::nodePool.get().setup("22", *rootNode);
        Engine::nodePool.get().setup("23", *rootNode);
        Engine::nodePool.get().setup("24", *rootNode);
        Engine::nodePool.get().setup("25", *rootNode);
        Engine::nodePool.get().setup("26", *rootNode);
        Engine::nodePool.get().setup("27", *rootNode);
        Engine::nodePool.get().setup("28", *rootNode);
        Engine::nodePool.get().setup("29", *rootNode);
        Engine::nodePool.get().setup("30", *rootNode);
        Engine::nodePool.get().setup("31", *rootNode);
        Engine::nodePool.get().setup("32", *rootNode);
        Engine::nodePool.get().setup("33", *rootNode);
        Engine::nodePool.get().setup("34", *rootNode);
        Engine::nodePool.get().setup("35", *rootNode);
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

        mainWindow.updateInternal(deltaTime);
        CursorSystem::draw();
    }

    void Editor::render2D(RenderTexture2D &renderTexture, const float deltaTime)
    {
        if (!_initialized) return;

        _viewportRenderTexture = &renderTexture;
        mainWindow.drawInternal(deltaTime);
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

    void Editor::closeProject() { _currentProjectPath.clear(); }

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

    Editor::Editor() : _uiRoot(mainWindow)
    {
    }
} // namespace BreadEditor

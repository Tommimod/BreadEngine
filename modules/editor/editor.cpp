#include "../engine/engine.h"
#include "editor.h"

#include "cursorSystem.h"
#include "node.h"

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
        if (_initialized) return true;

        GuiLoadStyleDefault();
        SetTraceLogLevel(LOG_ALL);
        _initialized = true;

        const auto rootNode = &Engine::getRootNode();
        auto &testNode = Engine::nodePool.get().setup("1", *rootNode);
        auto &testNode2 = Engine::nodePool.get().setup("1.1", testNode);
        auto &testNode25 = Engine::nodePool.get().setup("1.2", testNode);
        auto &testNode444 = Engine::nodePool.get().setup("1.1.1", testNode2);
        auto &testNode3 = Engine::nodePool.get().setup("2", *rootNode);
        return true;
    }

    void Editor::shutdown()
    {
        if (!_initialized) return;

        closeProject();
        _initialized = false;
    }

    void Editor::update(float deltaTime)
    {
        if (!_initialized) return;

        CursorSystem::draw();
    }

    void Editor::render2D(float deltaTime)
    {
        if (!_initialized) return;

        mainWindow.render2D(deltaTime);
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

    bool Editor::compileGame()
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
} // namespace BreadEditor

#pragma once
#include "windows/mainWindow/mainWindow.h"
#include <string>
#include "engine.h"

using namespace BreadEngine;

namespace BreadEditor {
    class Editor
    {
    public:
        MainWindow main_window;

        static Editor &getInstance();

        void setEngine(Engine &engine);

        bool initialize();

        void shutdown();

        void update(float deltaTime);

        void render2D(float deltaTime);

        void render3D(float deltaTime);

        // Project management
        bool createProject(const std::string &name, const std::string &path);

        bool openProject(const std::string &path);

        void closeProject();

        // Game compilation and running
        bool compileGame();

        bool runGame();

        void stopGame();

        [[nodiscard]] bool isProjectOpen() const { return !_currentProjectPath.empty(); }
        [[nodiscard]] const std::string &getCurrentProjectPath() const { return _currentProjectPath; }

    private:
        Editor() = default;

        ~Editor() = default;

        bool _initialized = false;
        std::string _currentProjectPath;
        Engine *_engine = nullptr;
    };
} // namespace BreadEditor

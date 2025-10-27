#pragma once
#include "windows/mainWindow/mainWindow.h"
#include <string>

using namespace BreadEngine;

namespace BreadEditor {
    class Editor
    {
    public:
        MainWindow main_window;

        static Editor &GetInstance();

        bool Initialize();

        void Shutdown();

        void Update(float deltaTime);

        void Render(float deltaTime);

        // Project management
        bool CreateProject(const std::string &name, const std::string &path);

        bool OpenProject(const std::string &path);

        void CloseProject();

        // Game compilation and running
        bool CompileGame();

        bool RunGame();

        void StopGame();

        [[nodiscard]] bool IsProjectOpen() const { return !currentProjectPath.empty(); }
        [[nodiscard]] const std::string &GetCurrentProjectPath() const { return currentProjectPath; }

    private:
        Editor() = default;

        ~Editor() = default;

        bool initialized = false;
        std::string currentProjectPath;
    };
} // namespace BreadEditor

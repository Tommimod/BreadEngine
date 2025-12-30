#pragma once
#include "engine.h"
#include "uitoolkit/uiWindow.h"
using namespace BreadEngine;

namespace BreadEditor {
    class WindowsModel
    {
    public:
        WindowsModel();

        ~WindowsModel();

        Action<std::string> onWindowOpened;
        Action<std::string> onWindowClosed;

        [[nodiscard]] const std::vector<std::string> &getNotOpenedWindowsNames() const { return _notOpenedWindowsNames; }

        void addWindowToAllowList(const std::string& id);
        void removeWindowFromAllowList(const std::string &id);
        std::function<UiWindow *()> getWindowFactory(const std::string &id);

    private:
        std::vector<std::string> _allWindows;
        std::vector<std::string> _notOpenedWindowsNames;
        std::vector<std::function<UiWindow *()>> _act;
    };
} // BreadEditor

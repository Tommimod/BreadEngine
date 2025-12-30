#include "windowsModel.h"
#include "windows/mainWindow/assetsWindow.h"
#include "windows/mainWindow/consoleWindow.h"
#include "windows/mainWindow/nodeInspectorWindow.h"
#include "windows/mainWindow/nodeTreeWindow.h"
#include "windows/mainWindow/viewportWindow.h"

namespace BreadEditor {
    WindowsModel::WindowsModel()
    {
        _allWindows = {
            AssetsWindow::Id,
            ConsoleWindow::Id,
            NodeTreeWindow::Id,
            NodeInspectorWindow::Id,
            ViewportWindow::Id,
        };

        _notOpenedWindowsNames = _allWindows;
        _act = {
            [] { return new AssetsWindow(AssetsWindow::Id); },
            [] { return new ConsoleWindow(ConsoleWindow::Id); },
            [] { return new NodeTreeWindow(NodeTreeWindow::Id); },
            [] { return new NodeInspectorWindow(NodeInspectorWindow::Id); },
            [] { return new ViewportWindow(ViewportWindow::Id); },
        };
    }

    WindowsModel::~WindowsModel()
    {
        onWindowOpened.unsubscribeAll();
        onWindowClosed.unsubscribeAll();
    }

    void WindowsModel::addWindowToAllowList(const std::string &id)
    {
        _notOpenedWindowsNames.emplace_back(id);
        onWindowClosed.invoke(id);
    }

    void WindowsModel::removeWindowFromAllowList(const std::string &id)
    {
        _notOpenedWindowsNames.erase(std::ranges::remove(_notOpenedWindowsNames, id).begin());
        onWindowOpened.invoke(id);
    }

    std::function<UiWindow *()> WindowsModel::getWindowFactory(const std::string &id)
    {
        const auto index = std::ranges::find(_allWindows, id);
        return _act[index - _allWindows.begin()];
    }
} // BreadEditor

#include "windowsModel.h"
#include "../windows/assetsWindow.h"
#include "../windows/consoleWindow.h"
#include "../windows/propertyInspectorWindow.h"
#include "../windows/nodeTreeWindow.h"
#include "../windows/viewportWindow.h"

namespace BreadEditor {
    WindowsModel::WindowsModel() = default;

    WindowsModel::~WindowsModel()
    {
        onWindowOpened.unsubscribeAll();
        onWindowClosed.unsubscribeAll();
    }

    void WindowsModel::initialize()
    {
        _allWindowsIds = {
            AssetsWindow::Id,
            ConsoleWindow::Id,
            NodeTreeWindow::Id,
            PropertyInspectorWindow::Id,
            ViewportWindow::Id
        };

        _notOpenedWindowsNames = _allWindowsIds;
        _act = {
            [this] { return getWindow(AssetsWindow::Id); },
            [this] { return getWindow(ConsoleWindow::Id); },
            [this] { return getWindow(NodeTreeWindow::Id); },
            [this] { return getWindow(PropertyInspectorWindow::Id); },
            [this] { return getWindow(ViewportWindow::Id); }
        };

        _windows = {
            new AssetsWindow(AssetsWindow::Id),
            new ConsoleWindow(ConsoleWindow::Id),
            new NodeTreeWindow(NodeTreeWindow::Id),
            new PropertyInspectorWindow(PropertyInspectorWindow::Id),
            new ViewportWindow(ViewportWindow::Id)
        };
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
        const auto index = std::ranges::find(_allWindowsIds, id) - _allWindowsIds.begin();
        return _act[static_cast<int>(index)];
    }

    UiWindow *WindowsModel::getWindow(const std::string &id)
    {
        const auto index = std::ranges::find(_allWindowsIds, id) - _allWindowsIds.begin();
        return _windows[static_cast<int>(index)];
    }
} // BreadEditor

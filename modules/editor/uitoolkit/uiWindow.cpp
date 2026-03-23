#include "uiWindow.h"
#include "editor.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string_view &id)
    {
        setup(id);
    }

    UiWindow::UiWindow(const std::string_view &id, UiElement *parentElement)
    {
        setup(id, parentElement);
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::update(const float deltaTime)
    {
        updateResizable(*this);
        UiScrollPanel::update(deltaTime);
    }

    void UiWindow::dispose()
    {
        UiWindow::unsubscribe();
        UiScrollPanel::dispose();
        setVerticalResized(false);
        setHorizontalResized(false);
        _isAwake = false;
    }

    void UiWindow::close()
    {
        _parent->destroyChild(this);
        getRootElement()->setDirty();
        Editor::getInstance().getEditorModel().getWindowsModel()->addWindowToAllowList(id);
    }

    void UiWindow::awake()
    {
        Editor::getInstance().getEditorModel().getWindowsModel()->removeWindowFromAllowList(id);
    }

    void UiWindow::subscribe()
    {
    }

    void UiWindow::unsubscribe()
    {
    }

    bool UiWindow::tryDeleteSelf()
    {
        return false;
    }
} // BreadEditor

#include "uiWindow.h"
#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string_view &id)
    {
        setup(id);
        _content = &UiPool::scrollPanelPool.get().setup(UiWindow::id + "_content", this);
        _content->setSizePercentPermanent({1,.995f});
        _content->setPosition({0, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT});
    }

    UiWindow::UiWindow(const std::string_view &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        _content = &UiPool::scrollPanelPool.get().setup(UiWindow::id + "_content", this);
        _content->setSizePercentPermanent({1,.995f});
        _content->setPosition({0, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT});
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::open() const
    {
        Editor::getInstance().getEditorModel().getWindowsModel().removeWindowFromAllowList(id);
    }

    void UiWindow::draw(const float deltaTime)
    {
        if (GuiWindowBox(_bounds, id.c_str()))
        {
            close();
        }
    }

    void UiWindow::update(const float deltaTime)
    {
        updateResizable(*this);
    }

    void UiWindow::dispose()
    {
        _content = nullptr;
        onClose.unsubscribeAll();
        UiWindow::unsubscribe();
        setVerticalResized(false);
        setHorizontalResized(false);
    }

    void UiWindow::close()
    {
        Editor::getInstance().getEditorModel().getWindowsModel().addWindowToAllowList(id);
        onClose.invoke(this);
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

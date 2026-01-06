#include "uiWindow.h"
#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string &id) : _closeButton(UiPool::buttonPool.get())
    {
        setup(id);
        UiWindow::initialize();
    }

    UiWindow::UiWindow(const std::string &id, UiElement *parentElement) : _closeButton(UiPool::buttonPool.get())
    {
        setup(id, parentElement);
        UiWindow::initialize();
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::draw(const float deltaTime)
    {
    }

    void UiWindow::update(const float deltaTime)
    {
        updateResizable(*this);
    }

    void UiWindow::dispose()
    {
        UiWindow::unsubscribe();
        Editor::getInstance().getEditorModel().getWindowsModel()->addWindowToAllowList(id);
        UiElement::dispose();
        setVerticalResized(false);
        setHorizontalResized(false);
    }

    void UiWindow::subscribe()
    {
        _closeButton.onClick.subscribe([this](UiButton *_)
        {
            _parent->destroyChild(this);
        });
    }

    void UiWindow::unsubscribe()
    {
        _closeButton.onClick.unsubscribeAll();
    }

    bool UiWindow::tryDeleteSelf()
    {
        return false;
    }

    void UiWindow::initialize()
    {
        _closeButton.setup(id + "closeButton", this, "X");
        _closeButton.setSize({15, 15});
        _closeButton.setAnchor(UI_RIGHT_TOP);
        _closeButton.setPivot({1, 1});
        _closeButton.setPosition({-5, 20});

        Editor::getInstance().getEditorModel().getWindowsModel()->removeWindowFromAllowList(id);
    }
} // BreadEditor

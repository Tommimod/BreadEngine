#include "uiWindow.h"
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

    UiWindow::~UiWindow()
    {
    }

    void UiWindow::draw(const float deltaTime)
    {
        if (!isActive) return;

        UiElement::draw(deltaTime);
        updateResizable(*this);
    }

    void UiWindow::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiWindow::dispose()
    {
        UiWindow::unsubscribe();
        UiElement::dispose();
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
        return true;
    }

    void UiWindow::initialize()
    {
        _closeButton.setup(id + "closeButton", this, "X");
        _closeButton.setSize({15, 15});
        _closeButton.setAnchor(UI_RIGHT_TOP);
        _closeButton.setPivot({1, 1});
        _closeButton.setPosition({-5, 20});
    }
} // BreadEditor

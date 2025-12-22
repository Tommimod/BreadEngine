#include "assetsWindow.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "mainWindowAssetsWindow";

    AssetsWindow::AssetsWindow(const std::string &id)
    {
        setup(id);
        isHorizontalResized = true;
        subscribe();
    }

    AssetsWindow::AssetsWindow(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        isHorizontalResized = true;
        subscribe();
    }

    AssetsWindow::~AssetsWindow()
    {
        unsubscribe();
    }

    void AssetsWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        updateResizable(*this);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void AssetsWindow::update(float deltaTime)
    {
        UiElement::update(deltaTime);
        updateResizable(*this);
    }

    void AssetsWindow::dispose()
    {
        UiElement::dispose();
    }

    bool AssetsWindow::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    void AssetsWindow::subscribe()
    {
    }

    void AssetsWindow::unsubscribe()
    {
    }
} // BreadEditor

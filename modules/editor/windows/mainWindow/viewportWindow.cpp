#include "viewportWindow.h"

namespace BreadEditor {
    std::string ViewportWindow::Id = "mainWindowViewportWindow";

    ViewportWindow::ViewportWindow(const std::string &id)
    {
        setup(id);
        subscribe();
    }

    ViewportWindow::ViewportWindow(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ViewportWindow::~ViewportWindow()
    {
        unsubscribe();
    }

    void ViewportWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void ViewportWindow::update(float deltaTime)
    {
        updateResizable(*this);
        UiElement::update(deltaTime);
    }

    void ViewportWindow::dispose()
    {
        UiElement::dispose();
    }

    bool ViewportWindow::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    void ViewportWindow::subscribe()
    {
    }

    void ViewportWindow::unsubscribe()
    {
    }
} // BreadEditor

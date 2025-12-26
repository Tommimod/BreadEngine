#include "viewportWindow.h"

namespace BreadEditor {
    std::string ViewportWindow::Id = "mainWindowViewportWindow";

    ViewportWindow::ViewportWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    ViewportWindow::ViewportWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ViewportWindow::~ViewportWindow()
    {
    }

    void ViewportWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void ViewportWindow::update(float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void ViewportWindow::dispose()
    {
        UiWindow::dispose();
    }

    void ViewportWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void ViewportWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }
} // BreadEditor

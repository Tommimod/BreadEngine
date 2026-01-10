#include "viewportWindow.h"

namespace BreadEditor {
    std::string ViewportWindow::Id = "Viewport";

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

    ViewportWindow::~ViewportWindow() = default;

    void ViewportWindow::draw(const float deltaTime)
    {
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
    }

    void ViewportWindow::update(const float deltaTime)
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

#include "assetsWindow.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "Assets";

    AssetsWindow::AssetsWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    AssetsWindow::AssetsWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    AssetsWindow::~AssetsWindow() = default;

    void AssetsWindow::draw(const float deltaTime)
    {
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
    }

    void AssetsWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void AssetsWindow::dispose()
    {
        UiWindow::dispose();
    }

    void AssetsWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void AssetsWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }
} // BreadEditor

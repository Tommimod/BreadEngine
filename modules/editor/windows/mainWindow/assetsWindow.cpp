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
        updateScrollView(_bounds, _bounds);
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

    void AssetsWindow::updateScrollView(const Rectangle &targetWidthRect, const Rectangle &targetHeightRect)
    {
        _contentView.x = _bounds.x;
        _contentView.y = _bounds.y;
        _contentView.height = _bounds.height - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 15;
        _contentView.width = _bounds.width - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 1;
    }
} // BreadEditor

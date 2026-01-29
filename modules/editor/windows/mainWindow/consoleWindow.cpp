#include "consoleWindow.h"

namespace BreadEditor {
    std::string ConsoleWindow::Id = "Console";

    ConsoleWindow::ConsoleWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    ConsoleWindow::ConsoleWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ConsoleWindow::~ConsoleWindow() = default;

    void ConsoleWindow::draw(const float deltaTime)
    {
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
    }

    void ConsoleWindow::update(const float deltaTime)
    {
        updateScrollView(_bounds, _bounds);
        UiWindow::update(deltaTime);
    }

    void ConsoleWindow::dispose()
    {
        UiWindow::dispose();
    }

    void ConsoleWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void ConsoleWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }

    void ConsoleWindow::updateScrollView(const Rectangle &targetWidthRect, const Rectangle &targetHeightRect)
    {
        _contentView.x = _bounds.x;
        _contentView.y = _bounds.y;
        _contentView.height = _bounds.height - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 15;
        _contentView.width = _bounds.width - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH) - GuiGetStyle(DEFAULT, BORDER_WIDTH) - 1;
    }
} // BreadEditor

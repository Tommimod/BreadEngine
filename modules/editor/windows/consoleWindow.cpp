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
} // BreadEditor

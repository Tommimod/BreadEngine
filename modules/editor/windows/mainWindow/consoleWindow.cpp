#include "consoleWindow.h"

namespace BreadEditor {
    std::string ConsoleWindow::Id = "mainWindowConsoleWindow";

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

    ConsoleWindow::~ConsoleWindow()
    {
    }

    void ConsoleWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void ConsoleWindow::update(float deltaTime)
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

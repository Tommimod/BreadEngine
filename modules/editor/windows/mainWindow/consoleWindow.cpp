#include "ConsoleWindow.h"

namespace BreadEditor {
    std::string ConsoleWindow::Id = "mainWindowConsoleWindow";

    ConsoleWindow::ConsoleWindow(const std::string &id)
    {
        setup(id);
        subscribe();
    }

    ConsoleWindow::ConsoleWindow(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ConsoleWindow::~ConsoleWindow()
    {
        unsubscribe();
    }

    void ConsoleWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void ConsoleWindow::update(float deltaTime)
    {
        UiElement::update(deltaTime);
        updateResizable(*this);
    }

    void ConsoleWindow::dispose()
    {
        UiElement::dispose();
    }

    bool ConsoleWindow::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }

    void ConsoleWindow::subscribe()
    {
    }

    void ConsoleWindow::unsubscribe()
    {
    }
} // BreadEditor

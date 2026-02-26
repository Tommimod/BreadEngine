#include "consoleWindow.h"

#include "editor.h"
#include "editorStyle.h"

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
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
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

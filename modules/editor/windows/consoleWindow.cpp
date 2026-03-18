#include "consoleWindow.h"
#include "editor.h"
#include "editorStyle.h"

namespace BreadEditor {
    std::string ConsoleWindow::Id = "Console";

    ConsoleWindow::ConsoleWindow(const std::string_view &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    ConsoleWindow::ConsoleWindow(const std::string_view &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    ConsoleWindow::~ConsoleWindow() = default;

    void ConsoleWindow::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiScrollPanel(_bounds, nullptr, _contentView, &_scrollPos, &_scrollView);
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
        _logSubscription = Logger::OnLog.subscribe([this](const Logger::LogLevel level, const std::string_view message) { onNewLogCreated(level, message); });
        UiWindow::subscribe();
    }

    void ConsoleWindow::unsubscribe()
    {
        Logger::OnLog.unsubscribe(_logSubscription);
        UiWindow::unsubscribe();
    }

    void ConsoleWindow::onNewLogCreated(Logger::LogLevel level, std::string_view message)
    {
    }
} // BreadEditor

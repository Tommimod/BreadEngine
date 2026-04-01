#include "consoleWindow.h"
#include "editor.h"
#include "editorStyle.h"
#include "logger.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string ConsoleWindow::Id = "Console";

    ConsoleWindow::ConsoleWindow(const std::string_view &id) : UiWindow(id)
    {
        setup(id);
    }

    ConsoleWindow::ConsoleWindow(const std::string_view &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
    }

    ConsoleWindow::~ConsoleWindow() = default;

    void ConsoleWindow::awake()
    {
        UiWindow::awake();
        float offset = 1;

        auto &panel = UiPool::panelPool.get().setup(id + "_panel", this);
        panel.setSizePercentPermanent({1, -1});
        panel.setSize({-1, 20});
        panel.setPosition({0, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT});

        auto &clearButton = UiPool::buttonPool.get().setup(id + "_clear_button", &panel, "Clear");
        clearButton.setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
        clearButton.setSize({35, -1});
        clearButton.setSizePercentPermanent({-1, 1});
        clearButton.onClick.subscribe([this](UiButton *)
        {
            clearLogs();
        });
        offset += clearButton.getSize().x;

        auto &infoLogsButton = UiPool::buttonPool.get().setup(id + "_info_button", &panel, GuiIconText(ICON_INFO, nullptr));
        infoLogsButton.setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
        infoLogsButton.setSize({20, -1});
        infoLogsButton.setSizePercentPermanent({-1, 1});
        infoLogsButton.setPosition({offset, 0});
        infoLogsButton.onClick.subscribe([this](UiButton *button)
        {
            switchInfoLogsVisibility(button);
        });
        infoLogsButton.setState(STATE_FOCUSED);
        offset += infoLogsButton.getSize().x;

        auto &warningLogsButton = UiPool::buttonPool.get().setup(id + "_warning_button", &panel, GuiIconText(ICON_WARNING, nullptr));
        warningLogsButton.setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
        warningLogsButton.setSize({20, -1});
        warningLogsButton.setSizePercentPermanent({-1, 1});
        warningLogsButton.setPosition({offset, 0});
        warningLogsButton.onClick.subscribe([this](UiButton *button)
        {
            switchWarningLogsVisibility(button);
        });
        warningLogsButton.setState(STATE_FOCUSED);
        offset += warningLogsButton.getSize().x;

        auto &errorLogsButton = UiPool::buttonPool.get().setup(id + "_error_button", &panel, GuiIconText(ICON_DEMON, nullptr));
        errorLogsButton.setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
        errorLogsButton.setSize({20, -1});
        errorLogsButton.setSizePercentPermanent({-1, 1});
        errorLogsButton.setPosition({offset, 0});
        errorLogsButton.onClick.subscribe([this](UiButton *button)
        {
            switchErrorLogsVisibility(button);
        });
        errorLogsButton.setState(STATE_FOCUSED);

        _content->setSizePercentPermanent({1, .7f});
        _textLogPanel = &UiPool::scrollPanelPool.get().setup(id + "_text_log_panel", this);
        _textLogPanel->setSizePercentPermanent({1, .3});
        _textLogPanel->setPosition({0, _content->getPosition().y + _content->getSize().y});
        _logText = &UiPool::labelPool.get().setup(id + "_log_text", _textLogPanel, "");
        _logText->setSizePercentPermanent({.95f, -1});
        _logText->setTextAlignment(TEXT_ALIGN_LEFT);
        _logText->setVerticalAlignment(TEXT_ALIGN_TOP);
        _logText->setWrapMode(TEXT_WRAP_WORD);
        _textLogPanel->calculateRectForScroll(_logText);

        subscribe();
    }

    void ConsoleWindow::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        UiWindow::draw(deltaTime);
    }

    void ConsoleWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void ConsoleWindow::dispose()
    {
        _logText = nullptr;
        _errorLogsVisible = true;
        _warningLogsVisible = true;
        _infoLogsVisible = true;
        _messages.clear();
        UiWindow::dispose();
    }

    void ConsoleWindow::subscribe()
    {
        _logSubscription = Logger::OnLog.subscribe([this](const Logger::LogEntity &entity) { onNewLogCreated(entity); });
        UiWindow::subscribe();
    }

    void ConsoleWindow::unsubscribe()
    {
        Logger::OnLog.unsubscribe(_logSubscription);
        UiWindow::unsubscribe();
    }

    void ConsoleWindow::rebuild()
    {
        for (const auto messageUiElement: _messages)
        {
            _content->destroyChild(messageUiElement);
        }

        _messages.clear();
        for (auto &allLogs = Logger::getLogs(); const auto &log: allLogs)
        {
            onNewLogCreated(log);
        }
    }

    void ConsoleWindow::onNewLogCreated(const Logger::LogEntity &entity)
    {
        if (entity.level == Logger::Info && !_infoLogsVisible) return;
        if (entity.level == Logger::Warning && !_warningLogsVisible) return;
        if (entity.level == Logger::Error && !_errorLogsVisible) return;

        constexpr float size = 35;
        float offset = 0;
        if (_messages.empty())
        {
            offset = getChildById(id + "_panel")->getSize().y;
        }
        else
        {
            offset = _messages.back()->getPosition().y + size;
        }

        auto &messageUiElement = UiPool::messageUiElementPool.get().setup(id + "_message", _content, entity);
        messageUiElement.setSize({-1, size});
        messageUiElement.setSizePercentPermanent({.99f, -1});
        messageUiElement.setPosition({2, offset});
        messageUiElement.onClick.subscribe([this](const Logger::LogEntity &logEntity)
        {
            _logText->setText(logEntity.message.data());
            const auto newLines = std::ranges::count(logEntity.message, '\n');
            _logText->setSize({-1, static_cast<float>(newLines) * static_cast<float>(_logText->getTextSize()) + 30});
            _textLogPanel->calculateRectForScroll(_logText);
        });

        _messages.emplace_back(&messageUiElement);
        _content->calculateRectForScroll(&messageUiElement);
    }

    void ConsoleWindow::clearLogs()
    {
        for (const auto messageUiElement: _messages)
        {
            _content->destroyChild(messageUiElement);
        }

        _messages.clear();
        _logText->setText("");
        Logger::clear();
    }

    void ConsoleWindow::switchInfoLogsVisibility(UiButton *switchButton)
    {
        _infoLogsVisible = !_infoLogsVisible;
        switchButton->setState(_infoLogsVisible ? STATE_FOCUSED : STATE_NORMAL);
        rebuild();
    }

    void ConsoleWindow::switchWarningLogsVisibility(UiButton *switchButton)
    {
        _warningLogsVisible = !_warningLogsVisible;
        switchButton->setState(_warningLogsVisible ? STATE_FOCUSED : STATE_NORMAL);
        rebuild();
    }

    void ConsoleWindow::switchErrorLogsVisibility(UiButton *switchButton)
    {
        _errorLogsVisible = !_errorLogsVisible;
        switchButton->setState(_errorLogsVisible ? STATE_FOCUSED : STATE_NORMAL);
        rebuild();
    }
} // BreadEditor

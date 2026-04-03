#pragma once
#include "logger.h"
#include "editorElements/messageUiElement.h"
#include "uitoolkit/uiButton.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiLabel.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class ConsoleWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit ConsoleWindow(const std::string_view &id);

        explicit ConsoleWindow(const std::string_view &id, UiElement *parentElement);

        ~ConsoleWindow() override;

        void awake() override;

        void dispose() override;

        const char *getTitle() override { return _title; }

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();
        UiScrollPanel *_textLogPanel = nullptr;
        UiLabel *_logText = nullptr;
        SubscriptionHandle _logSubscription;
        std::vector<MessageUiElement *> _messages;
        bool _infoLogsVisible = true;
        bool _warningLogsVisible = true;
        bool _errorLogsVisible = true;

        void rebuild();

        void onNewLogCreated(const Logger::LogEntity &entity);

        void clearLogs();

        void switchInfoLogsVisibility(UiButton *switchButton);

        void switchWarningLogsVisibility(UiButton *switchButton);

        void switchErrorLogsVisibility(UiButton *switchButton);
    };
} // BreadEditor

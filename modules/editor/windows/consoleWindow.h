#pragma once
#include "logger.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class ConsoleWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit ConsoleWindow(const std::string_view &id);

        explicit ConsoleWindow(const std::string_view &id, UiElement *parentElement);

        ~ConsoleWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        const char *getTitle() override { return _title; }

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();
        BreadEngine::SubscriptionHandle _logSubscription;

        void onNewLogCreated(BreadEngine::Logger::LogLevel level, std::string_view message);
    };
} // BreadEditor

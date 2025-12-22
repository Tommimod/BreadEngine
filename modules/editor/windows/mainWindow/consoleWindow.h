#pragma once
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiElement.h"

namespace BreadEditor {
    class ConsoleWindow final : public UiElement, IUiResizable
    {
    public:
        static std::string Id;

        explicit ConsoleWindow(const std::string &id);

        explicit ConsoleWindow(const std::string &id, UiElement *parentElement);

        ~ConsoleWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        const char *_title = "Console Window";
        Vector2 _scrollPos = {0.0f, 0.0f};
        Rectangle _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};

        void subscribe();

        void unsubscribe();
    };
} // BreadEditor

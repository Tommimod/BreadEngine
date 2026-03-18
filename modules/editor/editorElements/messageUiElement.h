#pragma once
#include "uitoolkit/uiButton.h"

namespace BreadEditor {
    class MessageUiElement : public UiButton
    {
    public:
        MessageUiElement();

        ~MessageUiElement() override;

        [[nodiscard]] MessageUiElement &setup(const std::string_view &id, UiElement *parentElement, const std::string_view &messageText);

        void draw(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

        std::string _messageText;
    };
} // BreadEditor

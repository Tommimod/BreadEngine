#pragma once
#include "uiElement.h"

namespace BreadEditor {
    class UiLabel final : public UiElement
    {
    public:
        UiLabel() = default;

        ~UiLabel() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiLabel &setup(const std::string &id, UiElement *parentElement, const std::string &text);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setTextSize(const int textSize) { _textSize = textSize; }

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
        GuiTextAlignment _textAlignment = TEXT_ALIGN_LEFT;
        int _textSize = 10;
    };
} // BreadEditor

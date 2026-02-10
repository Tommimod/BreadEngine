#pragma once
#include "action.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiLabelButton final : public UiElement
    {
    public:
        Action<UiLabelButton *> onClick;

        UiLabelButton();

        UiLabelButton &setup(const std::string &id, const std::string &newText);

        UiLabelButton &setup(const std::string &id, UiElement *parentElement, const std::string &newText);

        ~UiLabelButton() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setTextSize(const int textSize) { _textSize = textSize; }

        void setText(const std::string &newText) { _text = newText; }

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
        GuiTextAlignment _textAlignment = TEXT_ALIGN_LEFT;
        int _textSize = 10;
    };
} // namespace BreadEditor

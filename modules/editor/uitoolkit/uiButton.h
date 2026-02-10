#pragma once
#include "action.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiButton final : public UiElement
    {
    public:
        Action<UiButton *> onClick;

        UiButton();

        UiButton &setup(const std::string &id, const std::string &newText);

        UiButton &setup(const std::string &id, UiElement *parentElement, const std::string &newText);

        ~UiButton() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setTextSize(const int textSize) { _textSize = textSize; }

        void setText(const std::string &newText) { _text = newText; }

        void setClickOutside(const bool isCanClickOutside) { _canClickOutside = isCanClickOutside; }

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _canClickOutside = false;
        int _textSize = 10;
        std::string _text;
        GuiTextAlignment _textAlignment = TEXT_ALIGN_CENTER;
    };
} // namespace BreadEditor

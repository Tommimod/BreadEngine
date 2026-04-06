#pragma once
#include "action.h"
#include "editorStyle.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiLabelButton final : public UiElement
    {
    public:
        Action<UiLabelButton *> onClick;

        UiLabelButton();

        UiLabelButton &setup(const std::string_view &id, const std::string &newText);

        UiLabelButton &setup(const std::string_view &id, UiElement *parentElement, const std::string &newText);

        ~UiLabelButton() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setVerticalAlignment(const GuiTextAlignmentVertical alignment) { _verticalAlignment = alignment; }

        void setTextSize(const int textSize) { _textSize = textSize; }

        void setText(const std::string &newText) { _text = newText; }

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
        GuiTextAlignmentVertical _verticalAlignment = TEXT_ALIGN_MIDDLE;
        GuiTextAlignment _textAlignment = TEXT_ALIGN_LEFT;
        int _textSize = static_cast<int>(EditorStyle::FontSize::Medium);
    };
} // namespace BreadEditor

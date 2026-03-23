#pragma once
#include "editorStyle.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiLabel final : public UiElement
    {
    public:
        UiLabel() = default;

        ~UiLabel() override;

        void dispose() override;

        UiLabel &setup(const std::string_view &id, UiElement *parentElement, const std::string &text);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setVerticalAlignment(const GuiTextAlignmentVertical alignment) { _verticalAlignment = alignment; }

        void setWrapMode(const GuiTextWrapMode mode) { _wrapMode = mode; }

        void setTextSize(const int textSize) { _textSize = textSize; }

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
        GuiTextAlignment _textAlignment = TEXT_ALIGN_LEFT;
        GuiTextAlignmentVertical _verticalAlignment = TEXT_ALIGN_MIDDLE;
        GuiTextWrapMode _wrapMode = TEXT_WRAP_NONE;
        int _textSize = static_cast<int>(EditorStyle::FontSize::Medium);
    };
} // BreadEditor

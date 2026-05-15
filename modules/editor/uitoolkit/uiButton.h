#pragma once
#include "action.h"
#include "uiElement.h"
#include "editorStyle.h"

namespace BreadEditor {
    class UiButton : public UiElement
    {
    public:
        BreadEngine::Action<UiButton *> onClick;

        UiButton();

        UiButton &setup(const std::string_view &id, const std::string &newText);

        UiButton &setup(const std::string_view &id, UiElement *parentElement, const std::string &newText);

        ~UiButton() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void setTextVerticalAlignment(const GuiTextAlignmentVertical alignment) { _verticalAlignment = alignment; }

        void setTextSize(const int textSize) { _textSize = textSize; }

        void setText(const std::string &newText) { _text = newText; }

        void setClickOutside(const bool isCanClickOutside) { _canClickOutside = isCanClickOutside; }

    protected:
        bool tryDeleteSelf() override;

    private:
        GuiTextAlignment _textAlignment = TEXT_ALIGN_CENTER;
        GuiTextAlignmentVertical _verticalAlignment = TEXT_ALIGN_MIDDLE;
        std::string _text;
        int _textSize = static_cast<int>(EditorStyle::FontSize::Medium);
        bool _canClickOutside = false;
    };
} // namespace BreadEditor

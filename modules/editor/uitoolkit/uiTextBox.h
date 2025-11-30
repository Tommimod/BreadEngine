#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiTextBox final : public UiElement
    {
    public:
        BreadEngine::Action<char *> onValueChanged;
        BreadEngine::Action<char *, UiTextBox*> onValueChangedWithSender;

        UiTextBox();

        UiTextBox &setup(const std::string &id, const std::string &defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        ~UiTextBox() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setText(const std::string &newText);

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _internalText;
        char *_text = nullptr;
        int _textSize = 0;
        bool _editMode = false;
    };
} // BreadEditor

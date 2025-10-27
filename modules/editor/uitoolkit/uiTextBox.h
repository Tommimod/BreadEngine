#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiTextBox : public UiElement
    {
    public:
        BreadEngine::Action<char *> onTextChanged;

        UiTextBox();

        UiTextBox &setup(const std::string &id, std::string defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, std::string defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        ~UiTextBox() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;
        void setText(std::string newText);

    protected:
        void deleteSelf() override;

    private:
        std::string internalText;
        char *text = nullptr;
        int textSize;
        bool editMode;
    };
} // BreadEditor

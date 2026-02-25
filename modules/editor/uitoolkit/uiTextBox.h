#pragma once
#include "action.h"
#include "uiInspector.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiTextBox final : public UiElement
    {
    public:
        Action<char *> onValueChanged;
        Action<char *, UiTextBox *> onValueChangedWithSender;

        UiTextBox();

        ~UiTextBox() override;

        UiTextBox &setup(const std::string &id, const std::string &defaultText, int defaultTextSize = 10, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, int defaultTextSize = 10, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, std::function<std::string()> getFunc, int defaultTextSize = 10, bool defaultEditMode = false);

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setText(const std::string &newText);

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _editMode = false;
        int _textSize = 0;
        char *_text = nullptr;
        std::string _internalText;
        std::vector<char> _textBuffer;
        std::function<std::string()> _getFunc;
    };
} // BreadEditor

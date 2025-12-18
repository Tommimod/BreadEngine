#pragma once
#include "action.h"
#include "uiComponent.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiTextBox final : public UiElement
    {
    public:
        Action<char *> onValueChanged;
        Action<char *, UiTextBox *> onValueChangedWithSender;

        UiTextBox();

        UiTextBox &setup(const std::string &id, const std::string &defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, int defaultTextSize = 14, bool defaultEditMode = false);

        UiTextBox &setup(const std::string &id, UiElement *parentElement, UiComponent::PropWithComponent dynamicValue, int defaultTextSize = 14, bool defaultEditMode = false);

        ~UiTextBox() override;

        void dispose() override;

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
        UiComponent::PropWithComponent _dynamicValue{};
    };
} // BreadEditor

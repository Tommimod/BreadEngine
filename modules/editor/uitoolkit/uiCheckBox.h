#pragma once
#include "action.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiCheckBox final : public UiElement
    {
    public:
        Action<bool> onValueChanged;

        UiCheckBox();

        UiCheckBox &setup(const std::string &id, std::string checkBoxText, bool checked);

        UiCheckBox &setup(const std::string &id, UiElement *parentElement, std::string checkBoxText, bool checked);

        ~UiCheckBox() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setChecked(bool isChecked);

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _internalChecked = false;
        bool *_externalChecked = nullptr;
        std::string _text;
    };
} // BreadEditor

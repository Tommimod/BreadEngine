#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiCheckBox : public UiElement
    {
    public:
        BreadEngine::Action<bool> onStateChanged;

        UiCheckBox();

        UiCheckBox &setup(const std::string &id, bool checked, std::string checkBoxText);

        UiCheckBox &setup(const std::string &id, UiElement *parentElement, bool checked, std::string checkBoxText);

        ~UiCheckBox() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;
        void setChecked(bool isChecked);

    protected:
        void deleteSelf() override;

    private:
        bool internalChecked = false;
        bool *externalChecked = nullptr;
        std::string text;
    };
} // BreadEditor

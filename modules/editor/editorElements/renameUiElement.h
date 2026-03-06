#pragma once
#include "action.h"
#include "uitoolkit/uiButton.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiTextBox.h"

namespace BreadEditor {
    class RenameUiElement : public UiElement
    {
    public:
        Action<std::string &> onApply;

        RenameUiElement &setup(const std::string &fromName);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

        void awake() override;

    private:
        UiTextBox *_textBox = nullptr;
        UiButton *_applyButton = nullptr;
        std::string _text;
        const char* _title = "Rename";
    };
} // BreadEditor

#pragma once
#include "action.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiScrollPanel.h"
#include "uitoolkit/uiTextBox.h"

namespace BreadEditor {
    class UiLabelButton;

    class CustomDropdownUiElement : public UiElement
    {
    public:
        Action<std::string &> onOptionSelected;

        CustomDropdownUiElement &setup(const std::string_view &id, UiElement *parentElement, const std::vector<std::string> &options);

        void draw(float deltaTime) override;

        void dispose() override;

        void setOptions(const std::vector<std::string> &options);

        void close();

    protected:
        void awake() override;

    private:
        UiLabelButton *_lastClickedButton = nullptr;
        UiTextBox *_searchBox = nullptr;
        UiScrollPanel *_content = nullptr;

        std::vector<std::string> _options;

        void updateContent();
    };
} // BreadEditor

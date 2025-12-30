#pragma once
#include "uiButton.h"
#include "uiElement.h"
#include "uiLabelButton.h"
using namespace std;

namespace BreadEditor {
    class UiToolbar final : public UiElement
    {
    public:
        Action<int> onButtonPressed;
        Action<int> onButtonRequestedToRemove;

        UiToolbar();

        ~UiToolbar() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiToolbar &setup(const string &id, UiElement *parentElement, const vector<string> &buttonNames, bool isVisualAsLabel = false);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void replaceButtons(const vector<string> &buttonNames);

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isVisualAsLabel = false;

        void invokeButtonClicked(const UiButton *button);

        void invokeButtonClicked(const UiLabelButton *button);

        void invokeButtonRequestedToRemove(const UiLabelButton *button);
    };
} // namespace BreadEditor

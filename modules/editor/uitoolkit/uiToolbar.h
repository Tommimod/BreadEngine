#pragma once
#include "uiButton.h"
#include "uiElement.h"
#include "uiLabelButton.h"
using namespace std;

namespace BreadEditor {
    class UiToolbar final : public UiElement
    {
    public:
        Action<UiElement *> onButtonPressed;
        Action<UiElement *> onButtonRequestedToRemove;

        UiToolbar();

        ~UiToolbar() override;

        void dispose() override;

        UiToolbar &setup(const string &id, UiElement *parentElement, const vector<string> &buttonNames, bool isVisualAsLabel = false);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void replaceButtons(const vector<string> &buttonNames);

    protected:
        bool tryDeleteSelf() override;

    private:
        std::vector<UiElement *> _buttons {};
        bool _isVisualAsLabel = false;

        void invokeButtonClicked(UiButton *button);

        void invokeButtonClicked(UiLabelButton *button);

        void invokeButtonRequestedToRemove(const UiLabelButton *button);
    };
} // namespace BreadEditor

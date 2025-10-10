#pragma once
#include "uiElement.h"
using namespace std;

namespace BreadEditor
{
    class UiToolbar : public UiElement
    {
    public:
        UiToolbar();

        ~UiToolbar() override;

        UiToolbar &setup(const string &id, UiElement *parentElement, float height, const vector<string> &buttonNames);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        void deleteSelf() override;

    private:
        void onButtonClicked(const string &buttonName);
    };
} // namespace BreadEditor

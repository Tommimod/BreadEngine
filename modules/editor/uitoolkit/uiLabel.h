#pragma once
#include "uiElement.h"

namespace BreadEditor {
    class UiLabel final : public UiElement
    {
    public:
        UiLabel() = default;

        ~UiLabel() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiLabel *setup(const std::string &id, UiElement *parentElement, const std::string &text);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
    };
} // BreadEditor

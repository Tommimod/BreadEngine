#pragma once

#include "uiElement.h"

namespace BreadEditor {
    class UiPanel : public UiElement
    {
    public:
        UiPanel();

        UiPanel &setup(const std::string_view &id);

        UiPanel &setup(const std::string_view &id, UiElement *parentElement);

        ~UiPanel() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        const char *_title = nullptr;

        bool tryDeleteSelf() override;
    };
} // namespace BreadEditor

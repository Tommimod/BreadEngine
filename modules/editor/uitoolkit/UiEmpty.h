#pragma once
#include "IUiResizable.h"
#include "uiElement.h"
#include "windows/mainWindow/nodeTree.h"

namespace BreadEditor {
    class UiEmpty final : public UiElement, public IUiResizable
    {
    public:
        UiEmpty() = default;

        ~UiEmpty() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiEmpty *setup(const std::string &id);

        UiEmpty *setup(const std::string &id, UiElement *parentElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;
    };
} // BreadEditor

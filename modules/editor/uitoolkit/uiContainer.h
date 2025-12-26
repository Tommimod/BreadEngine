#pragma once
#include "IUiResizable.h"
#include "uiElement.h"
#include "windows/mainWindow/nodeTree.h"

namespace BreadEditor {
    class UiContainer final : public UiElement, public IUiResizable
    {
    public:
        UiContainer(LAYOUT_TYPE layoutType);

        ~UiContainer() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiContainer *setup(const std::string &id);

        UiContainer *setup(const std::string &id, UiElement *parentElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;
    };
} // BreadEditor

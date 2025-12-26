#pragma once
#include "IUiResizable.h"
#include "uiElement.h"
#include "uiToolbar.h"
#include "windows/mainWindow/nodeTreeWindow.h"

namespace BreadEditor {
    class UiContainer final : public UiElement, public IUiResizable
    {
    public:
        explicit UiContainer(LAYOUT_TYPE layoutType);

        ~UiContainer() override;

        void dispose() override
        {
            UiElement::dispose();
        }

        UiContainer *setup(const std::string &id);

        UiContainer *setup(const std::string &id, UiElement *parentElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void addChild(UiElement *child) override;

        void destroyChild(UiElement *child) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        UiToolbar &_toolbar;
        vector<std::string> _windowsOptions{};
        float _toolbarHeightInPercent = 0;

        void recalculateChilds();
    };
} // BreadEditor

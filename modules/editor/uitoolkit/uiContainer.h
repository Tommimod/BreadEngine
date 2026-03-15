#pragma once
#include "IUiResizable.h"
#include "uiElement.h"
#include "uiToolbar.h"
#include "../windows/nodeTreeWindow.h"

namespace BreadEditor {
    class UiContainer final : public UiElement, public IUiResizable
    {
    public:
        explicit UiContainer(LAYOUT_TYPE layoutType);

        ~UiContainer() override = default;

        void dispose() override;

        UiContainer *setup(const std::string &id);

        UiContainer *setup(const std::string &id, UiElement *parentElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void addChild(UiElement *child) override;

        void destroyChild(UiElement *child) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        UiToolbar *_toolbar = nullptr;
        std::unordered_map<std::string, std::string> _tabIdToWindowId{};
        vector<std::string> _tabs{};

        void initialize();

        void recalculateChilds();

        void onTabChanged(UiElement *uiElement);

        void onTabClosed(UiElement *uiElement);
    };
} // BreadEditor

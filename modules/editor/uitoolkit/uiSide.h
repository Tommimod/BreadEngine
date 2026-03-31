#pragma once
#include "IUiResizable.h"
#include "uiElement.h"
#include "uiPanel.h"
#include "../windows/nodeTreeWindow.h"

namespace BreadEditor {
    class UiSide final : public UiScrollPanel, public IUiResizable
    {
    public:
        explicit UiSide(LAYOUT_TYPE layoutType);

        ~UiSide() override = default;

        void dispose() override;

        UiSide *setup(const std::string_view &id);

        UiSide *setup(const std::string_view &id, UiElement *parentElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void addChild(UiElement *child) override;

        void destroyChild(UiElement *child) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        std::vector<UiWindow *> _slots;
        UiPanel *_topPanel = nullptr;

        void initializeElements();

        void recalculateChilds();
    };
} // BreadEditor

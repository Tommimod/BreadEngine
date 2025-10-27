#include "uiPool.h"

#include "engine.h"

namespace BreadEditor {
    auto UiLabelButtonFactory = []() -> UiLabelButton *
    {
        return new UiLabelButton();
    };
    ObjectPool<UiLabelButton> UiPool::labelButtonPool(UiLabelButtonFactory, 10);

    auto UiPanelFactory = []() -> UiPanel *
    {
        return new UiPanel();
    };
    ObjectPool<UiPanel> UiPool::panelPool(UiPanelFactory, 10);

    auto UiToolbarFactory = []() -> UiToolbar *
    {
        return new UiToolbar();
    };
    ObjectPool<UiToolbar> UiPool::toolbarPool(UiToolbarFactory, 2);

    auto NodeInstanceFactory = []() -> NodeUiElement *
    {
        return new NodeUiElement();
    };
    ObjectPool<NodeUiElement> UiPool::nodeUiElementPool(NodeInstanceFactory, 10);

    auto UiCheckBoxFactory = []() -> UiCheckBox *
    {
        return new UiCheckBox();
    };
    ObjectPool<UiCheckBox> UiPool::checkBoxPool(UiCheckBoxFactory, 10);

    auto UiTextBoxFactory = []() -> UiTextBox *
    {
        return new UiTextBox();
    };
    ObjectPool<UiTextBox> UiPool::textBoxPool(UiTextBoxFactory, 10);
} // BreadEditor

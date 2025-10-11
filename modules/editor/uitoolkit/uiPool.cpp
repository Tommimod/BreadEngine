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
    ObjectPool<UiToolbar> UiPool::toolbarPool(UiToolbarFactory, 10);

    auto NodeInstanceFactory = []() -> NodeUiElement *
    {
        return new NodeUiElement();
    };
    ObjectPool<NodeUiElement> UiPool::nodeInstancePool(NodeInstanceFactory, 10);

    auto InteractiveLineFactory = []() -> UiInteractiveLine *
    {
        return new UiInteractiveLine();
    };
    ObjectPool<UiInteractiveLine> UiPool::interactiveLinePool(InteractiveLineFactory, 10);
} // BreadEditor

#include "uiPool.h"

#include "engine.h"

namespace BreadEditor {
    auto UiLabelButtonFactory = []() -> UiLabelButton *
    {
        return new UiLabelButton();
    };
    ObjectPool<UiLabelButton> UiPool::labelButtonPool(UiLabelButtonFactory, 10);

    auto UiButtonFactory = []() -> UiButton *
    {
        return new UiButton();
    };
    ObjectPool<UiButton> UiPool::buttonPool(UiButtonFactory, 10);

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

    auto UiNumberBoxFactory = []() -> UiNumberBox *
    {
        return new UiNumberBox();
    };
    ObjectPool<UiNumberBox> UiPool::numberBoxPool(UiNumberBoxFactory, 10);

    auto UiComponentFactory = []() -> UiComponent *
    {
        return new UiComponent();
    };
    ObjectPool<UiComponent> UiPool::componentPool(UiComponentFactory, 10);

    auto UiLabelFactory = []() -> UiLabel *
    {
        return new UiLabel();
    };
    ObjectPool<UiLabel> UiPool::labelPool(UiLabelFactory, 10);

    auto UiVector2DFactory = []() -> UiVector2D *
    {
        return new UiVector2D();
    };
    ObjectPool<UiVector2D> UiPool::vector2DPool(UiVector2DFactory, 10);

    auto UiVector3DFactory = []() -> UiVector3D *
    {
        return new UiVector3D();
    };
    ObjectPool<UiVector3D> UiPool::vector3DPool(UiVector3DFactory, 10);

    auto UiVector4DFactory = []() -> UiVector4D *
    {
        return new UiVector4D();
    };
    ObjectPool<UiVector4D> UiPool::vector4DPool(UiVector4DFactory, 10);
} // BreadEditor

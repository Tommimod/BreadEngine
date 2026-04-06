#include "uiPool.h"
#include "engine.h"

namespace BreadEditor {
    auto UiEmptyElementFactory = []() -> UiEmpty *
    {
        return new UiEmpty();
    };
    ObjectPool<UiEmpty> UiPool::emptyElementPool(UiEmptyElementFactory, 1);

    auto UiLabelButtonFactory = []() -> UiLabelButton *
    {
        return new UiLabelButton();
    };
    ObjectPool<UiLabelButton> UiPool::labelButtonPool(UiLabelButtonFactory, 1);

    auto UiButtonFactory = []() -> UiButton *
    {
        return new UiButton();
    };
    ObjectPool<UiButton> UiPool::buttonPool(UiButtonFactory, 1);

    auto UiPanelFactory = []() -> UiPanel *
    {
        return new UiPanel();
    };
    ObjectPool<UiPanel> UiPool::panelPool(UiPanelFactory, 1);

    auto UiToolbarFactory = []() -> UiToolbar *
    {
        return new UiToolbar();
    };
    ObjectPool<UiToolbar> UiPool::toolbarPool(UiToolbarFactory, 2);

    auto NodeInstanceFactory = []() -> NodeUiElement *
    {
        return new NodeUiElement();
    };
    ObjectPool<NodeUiElement> UiPool::nodeUiElementPool(NodeInstanceFactory, 1);

    auto UiCheckBoxFactory = []() -> UiCheckBox *
    {
        return new UiCheckBox();
    };
    ObjectPool<UiCheckBox> UiPool::checkBoxPool(UiCheckBoxFactory, 1);

    auto UiTextBoxFactory = []() -> UiTextBox *
    {
        return new UiTextBox();
    };
    ObjectPool<UiTextBox> UiPool::textBoxPool(UiTextBoxFactory, 1);

    auto UiNumberBoxFactory = []() -> UiNumberBox *
    {
        return new UiNumberBox();
    };
    ObjectPool<UiNumberBox> UiPool::numberBoxPool(UiNumberBoxFactory, 1);

    auto UiComponentFactory = []() -> UiInspector *
    {
        return new UiInspector();
    };
    ObjectPool<UiInspector> UiPool::componentPool(UiComponentFactory, 1);

    auto UiLabelFactory = []() -> UiLabel *
    {
        return new UiLabel();
    };
    ObjectPool<UiLabel> UiPool::labelPool(UiLabelFactory, 1);

    auto UiVector2DFactory = []() -> UiVector2D *
    {
        return new UiVector2D();
    };
    ObjectPool<UiVector2D> UiPool::vector2DPool(UiVector2DFactory, 1);

    auto UiVector3DFactory = []() -> UiVector3D *
    {
        return new UiVector3D();
    };
    ObjectPool<UiVector3D> UiPool::vector3DPool(UiVector3DFactory, 1);

    auto UiVector4DFactory = []() -> UiVector4D *
    {
        return new UiVector4D();
    };
    ObjectPool<UiVector4D> UiPool::vector4DPool(UiVector4DFactory, 1);

    auto UiDropdownFactory = []() -> UiDropdown *
    {
        return new UiDropdown();
    };
    ObjectPool<UiDropdown> UiPool::dropdownPool(UiDropdownFactory, 1);

    auto UIFolderElementFactory = []() -> FolderUiElement *
    {
        return new FolderUiElement();
    };
    ObjectPool<FolderUiElement> UiPool::folderUiElementPool(UIFolderElementFactory, 10);

    auto UiScrollPanelFactory = []() -> UiScrollPanel *
    {
        return new UiScrollPanel();
    };
    ObjectPool<UiScrollPanel> UiPool::scrollPanelPool(UiScrollPanelFactory, 1);

    auto UIFileElementFactory = []() -> FileUiElement *
    {
        return new FileUiElement();
    };
    ObjectPool<FileUiElement> UiPool::fileUiElementPool(UIFileElementFactory, 10);

    auto UiRenameFactory = []() -> RenameUiElement *
    {
        return new RenameUiElement();
    };
    ObjectPool<RenameUiElement> UiPool::renameUiElementPool(UiRenameFactory, 1);

    auto MessageUiElementFactory = []() -> MessageUiElement *
    {
        return new MessageUiElement();
    };
    ObjectPool<MessageUiElement> UiPool::messageUiElementPool(MessageUiElementFactory, 1);
} // BreadEditor

#pragma once
#include "uiPanel.h"
#include "uiToolbar.h"
#include "uiLabelButton.h"
#include "../editorElements/nodeUiElement.h"
#include "objectPool.h"
#include "uiButton.h"
#include "uiCheckBox.h"
#include "uiComponent.h"
#include "uiDropdown.h"
#include "uiLabel.h"
#include "uiNumberBox.h"
#include "uiTextBox.h"
#include "uiVector2D.h"
#include "uiVector3D.h"
#include "uiVector4D.h"
#include "editorElements/folderUiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiPool
    {
    public:
        static ObjectPool<UiLabelButton> labelButtonPool;
        static ObjectPool<UiButton> buttonPool;
        static ObjectPool<UiPanel> panelPool;
        static ObjectPool<UiToolbar> toolbarPool;
        static ObjectPool<NodeUiElement> nodeUiElementPool;
        static ObjectPool<UiCheckBox> checkBoxPool;
        static ObjectPool<UiTextBox> textBoxPool;
        static ObjectPool<UiNumberBox> numberBoxPool;
        static ObjectPool<UiComponent> componentPool;
        static ObjectPool<UiLabel> labelPool;
        static ObjectPool<UiVector2D> vector2DPool;
        static ObjectPool<UiVector3D> vector3DPool;
        static ObjectPool<UiVector4D> vector4DPool;
        static ObjectPool<UiDropdown> dropdownPool;
        static ObjectPool<FolderUiElement> folderUiElementPool;
    };
} // BreadEditor

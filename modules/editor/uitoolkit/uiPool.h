#pragma once
#include "uiPanel.h"
#include "uiToolbar.h"
#include "uiLabelButton.h"
#include "windows/mainWindow/nodeUiElement.h"
#include "objectPool.h"
#include "uiCheckBox.h"
#include "uiComponent.h"
#include "uiLabel.h"
#include "uiTextBox.h"
#include "uiVector2D.h"
#include "uiVector3D.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiPool
    {
    public:
        static ObjectPool<UiLabelButton> labelButtonPool;
        static ObjectPool<UiPanel> panelPool;
        static ObjectPool<UiToolbar> toolbarPool;
        static ObjectPool<NodeUiElement> nodeUiElementPool;
        static ObjectPool<UiCheckBox> checkBoxPool;
        static ObjectPool<UiTextBox> textBoxPool;
        static ObjectPool<UiComponent> componentPool;
        static ObjectPool<UiLabel> labelPool;
        static ObjectPool<UiVector2D> vector2DPool;
        static ObjectPool<UiVector3D> vector3DPool;
    };
} // BreadEditor

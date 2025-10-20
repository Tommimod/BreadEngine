#pragma once
#include "uiPanel.h"
#include "uiToolbar.h"
#include "uiLabelButton.h"
#include "windows/mainWindow/nodeUiElement.h"
#include "objectPool.h"
using namespace BreadEngine;

namespace BreadEditor
{
    class UiPool
    {
    public:
        static ObjectPool<UiLabelButton> labelButtonPool;
        static ObjectPool<UiPanel> panelPool;
        static ObjectPool<UiToolbar> toolbarPool;
        static ObjectPool<NodeUiElement> nodeUiElementPool;
    };
} // BreadEditor

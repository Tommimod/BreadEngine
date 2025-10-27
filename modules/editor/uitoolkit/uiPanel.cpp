#include "uiPanel.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiPanel::UiPanel() = default;

    UiPanel &UiPanel::setup(const std::string &id)
    {
        return dynamic_cast<UiPanel &>(UiElement::setup(id));
    }

    UiPanel &UiPanel::setup(const std::string &id, UiElement *parentElement)
    {
        return dynamic_cast<UiPanel &>(UiElement::setup(id, parentElement));
    }

    UiPanel::~UiPanel() = default;

    void UiPanel::draw(const float deltaTime)
    {
        GuiSetState(state);
        GuiPanel(bounds, title);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiPanel::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiPanel::deleteSelf()
    {
        UiPool::panelPool.release(*this);
    }
} // namespace BreadEditor

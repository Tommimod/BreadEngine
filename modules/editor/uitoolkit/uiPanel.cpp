#include "uiPanel.h"
#include "editor.h"
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

    UiPanel::~UiPanel()
    {
        delete _title;
    }

    void UiPanel::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiPanel(_bounds, _title);
    }

    void UiPanel::update(const float deltaTime)
    {
    }

    bool UiPanel::tryDeleteSelf()
    {
        UiPool::panelPool.release(*this);
        return true;
    }
} // namespace BreadEditor

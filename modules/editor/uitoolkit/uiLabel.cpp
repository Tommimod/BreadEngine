#include "uiLabel.h"
#include "uiPool.h"

namespace BreadEditor {
    UiLabel::~UiLabel() = default;

    UiLabel *UiLabel::setup(const std::string &id, UiElement *parentElement, const std::string &text)
    {
        UiElement::setup(id, parentElement);
        _text = text;
        return this;
    }

    void UiLabel::draw(const float deltaTime)
    {
        if (!isActive) return;

        GuiSetStyle(DEFAULT, TEXT_SIZE, _textSize);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, _textAlignment);
        GuiLabel(_bounds, _text.c_str());
        UiElement::draw(deltaTime);
    }

    void UiLabel::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    bool UiLabel::tryDeleteSelf()
    {
        UiPool::labelPool.release(*this);
        return true;
    }
} // BreadEditor

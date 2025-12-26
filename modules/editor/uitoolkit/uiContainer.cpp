#include "uiContainer.h"

namespace BreadEditor {
    UiContainer::UiContainer(const LAYOUT_TYPE layoutType)
    {
        _layoutType = layoutType;
    }

    UiContainer::~UiContainer()
    = default;

    UiContainer *UiContainer::setup(const std::string &id)
    {
        UiElement::setup(id);
        return this;
    }

    UiContainer *UiContainer::setup(const std::string &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        return this;
    }

    void UiContainer::draw(const float deltaTime)
    {
        UiElement::draw(deltaTime);
    }

    void UiContainer::update(const float deltaTime)
    {
        updateResizable(*this);
        UiElement::update(deltaTime);
    }

    bool UiContainer::tryDeleteSelf()
    {
        return true;
    }
} // BreadEditor

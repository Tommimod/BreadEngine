#include "uiInteractiveLine.h"

namespace BreadEditor {
    UiInteractiveLine::UiInteractiveLine() = default;

    UiInteractiveLine & UiInteractiveLine::setup(const std::string &id, InteractiveLineType type)
    {
        UiElement::setup(id);
        interactiveType = type;
        return *this;
    }

    UiInteractiveLine & UiInteractiveLine::setup(const std::string &id, UiElement *parentElement, InteractiveLineType type)
    {
        UiElement::setup(id, parentElement);
        interactiveType = type;
        return *this;
    }

    UiInteractiveLine::~UiInteractiveLine()
    {
    }

    void UiInteractiveLine::draw(float deltaTime)
    {
        UiElement::draw(deltaTime);
    }

    void UiInteractiveLine::update(float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiInteractiveLine::deleteSelf()
    {
    }
} // BreadEditor
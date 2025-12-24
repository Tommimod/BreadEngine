#include "UiEmpty.h"

namespace BreadEditor {
    UiEmpty::~UiEmpty()
    = default;

    UiEmpty *UiEmpty::setup(const std::string &id)
    {
        UiElement::setup(id);
        return this;
    }

    UiEmpty *UiEmpty::setup(const std::string &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        return this;
    }

    void UiEmpty::draw(const float deltaTime)
    {
        UiElement::draw(deltaTime);
    }

    void UiEmpty::update(const float deltaTime)
    {
        updateResizable(*this);
        UiElement::update(deltaTime);
    }

    bool UiEmpty::tryDeleteSelf()
    {
        return true;
    }
} // BreadEditor

#include "uiNodeLink.h"

namespace BreadEditor {
    UiNodeLink &UiNodeLink::setup(const std::string_view &id, BreadEngine::Component *component)
    {
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, std::function<BreadEngine::Component *()> getFunc)
    {
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, UiElement *parentElement, BreadEngine::Component *component)
    {
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, UiElement *parentElement, std::function<BreadEngine::Component *()> getFunc)
    {
    }

    void UiNodeLink::draw(float deltaTime)
    {
        UiElement::draw(deltaTime);
    }

    void UiNodeLink::update(float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiNodeLink::dispose()
    {
        _getFunc = nullptr;
        _component = nullptr;
        onValueChanged.unsubscribeAll();
        UiElement::dispose();
    }

    void UiNodeLink::awake()
    {
        UiElement::awake();
    }

    bool UiNodeLink::tryDeleteSelf()
    {
        return UiElement::tryDeleteSelf();
    }
} // BreadEditor

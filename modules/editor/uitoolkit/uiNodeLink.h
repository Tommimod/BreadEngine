#pragma once
#include <functional>
#include "uiElement.h"
#include "core/component.h"

namespace BreadEditor {
    class UiNodeLink : public UiElement
    {
    public:
        BreadEngine::Action<BreadEngine::Component *> onValueChanged;

        UiNodeLink() = default;

        ~UiNodeLink() override = default;

        UiNodeLink &setup(const std::string_view &id, BreadEngine::Component *component);

        UiNodeLink &setup(const std::string_view &id, std::function<BreadEngine::Component *()> getFunc);

        UiNodeLink &setup(const std::string_view &id, UiElement *parentElement, BreadEngine::Component *component);

        UiNodeLink &setup(const std::string_view &id, UiElement *parentElement, std::function<BreadEngine::Component *()> getFunc);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        void awake() override;

        bool tryDeleteSelf() override;

    private:
        std::function<BreadEngine::Component *()> _getFunc;
        BreadEngine::Component *_component = nullptr;
    };
} // BreadEditor

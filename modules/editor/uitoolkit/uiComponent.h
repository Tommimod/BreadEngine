#pragma once
#include <typeindex>
#include "action.h"
#include "component.h"
#include "uiElement.h"
#include "property.h"

using namespace BreadEngine;

namespace BreadEditor {
    class UiComponent final : public UiElement
    {
    public:
        Action<std::type_index> onDelete;

        UiComponent();

        UiComponent &setup(const std::string &id, UiElement *parentElement);

        ~UiComponent() override;

        void dispose() override;

        void trackComponent(Component *component);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        [[nodiscard]] Component *getTrackedComponent() const;

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isTransform = false;
        std::string _componentName;
        std::vector<Property> _properties{};
        Component *_component = nullptr;

        void cleanUp();
    };
} // BreadEditor

#pragma once
#include <functional>
#include <typeindex>
#include "uiElement.h"
#include "uiLabelButton.h"
#include "core/component.h"

namespace BreadEditor {
    class UiNodeLink : public UiElement
    {
    public:
        Action<Component *> onValueChanged;

        UiNodeLink() = default;

        ~UiNodeLink() override = default;

        UiNodeLink &setup(const std::string_view &id, Component *component);

        UiNodeLink &setup(const std::string_view &id, std::function<Component *()> getFunc);

        UiNodeLink &setup(const std::string_view &id, UiElement *parentElement, Component *component);

        UiNodeLink &setup(const std::string_view &id, UiElement *parentElement, std::function<Component *()> getFunc);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setComponent(Component *component);

        void setExpectedType(const std::type_index &typeIndex);

        [[nodiscard]] bool isComponentTypeValid(Component *component) const;

        [[nodiscard]] std::string getExpectedTypeName() const;

    protected:
        void awake() override;

        bool tryDeleteSelf() override;

    private:
        std::function<Component *()> _getFunc;
        Component *_component = nullptr;
        UiLabelButton *_labelButton = nullptr;
        std::type_index _expectedType{typeid(void)}; // For type validation
    };
} // BreadEditor

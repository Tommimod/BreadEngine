#pragma once
#include <typeindex>
#include "action.h"
#include "component.h"
#include "uiElement.h"
#include "inspectorObject.h"

using namespace BreadEngine;

namespace BreadEditor {
    class UiInspector final : public UiElement
    {
    public:
        struct PropsOfStruct
        {
            std::unique_ptr<Property> property;
            InspectorStruct* inspectorStruct;
        };

        Action<std::type_index> onDelete;

        UiInspector();

        UiInspector &setup(const std::string &id, UiElement *parentElement);

        ~UiInspector() override;

        void dispose() override;

        void track(InspectorStruct* inspectorStruct);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _isTransform = false;
        std::string _componentName;
        std::vector<Property> _properties{};
        InspectorStruct *_inspectorStruct = nullptr;

        void cleanUp();
    };
} // BreadEditor

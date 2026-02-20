#pragma once
#include <map>
#include <typeindex>
#include "action.h"
#include "component.h"
#include "uiElement.h"
#include "inspectorObject.h"
#include "uiLabelButton.h"

using namespace BreadEngine;

namespace BreadEditor {
    class UiInspector final : public UiElement
    {
    public:
        Action<std::type_index> onDelete;

        UiInspector();

        UiInspector &setup(const std::string &id, UiElement *parentElement, bool isStatic);

        ~UiInspector() override;

        void dispose() override;

        void track(InspectorStruct *inspectorStruct);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        struct UiListData
        {
            bool isExpanded;
            std::shared_ptr<VectorAccessor> accessor;

            UiListData() : isExpanded(false), accessor(nullptr)
            {
            }

            UiListData(const bool isExpanded, const std::shared_ptr<VectorAccessor> *shared)
            {
                this->isExpanded = isExpanded;
                this->accessor = *shared;
            }

            ~UiListData() = default;
        };

        bool _isStatic = false;
        bool _isPermanent = false;
        std::string _componentName;
        InspectorStruct *_inspectorStruct = nullptr;
        std::map<Property *, UiListData> _uiListData{};

        void cleanUp();

        void initializeProperties(InspectorStruct *inspectorStruct, std::vector<Property> &properties, int &depth, float horizonDepth);

        void createSingleElement(int order, InspectorStruct *inspectorStruct, Property &property, VectorAccessor *vectorAccessor, int vectorIndex, int &depth, float horizonDepth);
    };
} // BreadEditor

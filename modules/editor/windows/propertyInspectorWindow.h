#pragma once
#include <map>
#include "node.h"
#include "uitoolkit/uiCheckBox.h"
#include "uitoolkit/uiInspector.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiTextBox.h"
#include "uitoolkit/uiWindow.h"
using namespace BreadEngine;

namespace BreadEditor {
    class PropertyInspectorWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit PropertyInspectorWindow(const std::string_view &id);

        explicit PropertyInspectorWindow(const std::string_view &id, UiElement *parentElement);

        ~PropertyInspectorWindow() override;

        void dispose() override;

        void lookupNode(Node *node);

        void lookupStruct(InspectorStruct *inspectorStruct);

        void clear();

        [[nodiscard]] const char *getTitle() override { return _title; }

        void awake() override;

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();

        std::map<UiElement *, SubscriptionHandle> _subscriptions{};
        Node *_engineNode = nullptr;
        InspectorStruct *_inspectorStruct = nullptr;
        UiCheckBox *_activeCheckBox = nullptr;
        UiTextBox *_nameTextBox = nullptr;
        std::vector<Component *> _trackedComponents{};
        std::vector<UiInspector *> _uiComponentElements{};

        void resetElementsState(bool withGlobalSettings);

        void rebuild();

        void onNodeActiveChanged(bool isActive) const;

        void onNodeNameChanged(const char *name) const;

        void setupUiComponent(UiInspector *uiComponentElement) const;

        void onUiComponentDeleted(std::type_index type);
    };
} // BreadEditor

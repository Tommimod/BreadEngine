#pragma once
#include <map>
#include "node.h"
#include "uitoolkit/uiCheckBox.h"
#include "uitoolkit/uiComponent.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiTextBox.h"
#include "uitoolkit/uiWindow.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeInspectorWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit NodeInspectorWindow(const std::string &id);

        explicit NodeInspectorWindow(const std::string &id, UiElement *parentElement);

        ~NodeInspectorWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void lookupNode(Node *node);

        void clear();

        const char *getTitle() override { return _title; }

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        std::map<UiElement *, SubscriptionHandle> _subscriptions{};

        const char *_title = "Inspector";
        Node *_engineNode = nullptr;
        UiCheckBox *_activeCheckBox = nullptr;
        UiTextBox *_nameTextBox = nullptr;
        std::vector<Component *> _trackedComponents{};
        std::vector<UiComponent *> _uiComponentElements{};

        void initialize() override;

        void resetElementsState();

        void onNodeActiveChanged(bool isActive) const;

        void onNodeNameChanged(const char *name) const;

        void setupUiComponent(UiComponent *uiComponentElement) const;

        void onUiComponentDeleted(std::type_index type);
    };
} // BreadEditor

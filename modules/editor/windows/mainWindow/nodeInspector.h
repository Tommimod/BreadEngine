#pragma once
#include <map>
#include "node.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiCheckBox.h"
#include "uitoolkit/uiComponent.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiTextBox.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeInspector final : public UiElement, public IUiResizable
    {
    public:
        static std::string Id;

        explicit NodeInspector(const std::string &id);

        explicit NodeInspector(const std::string &id, UiElement *parentElement);

        ~NodeInspector() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override
        {
            UiElement::dispose();
        }

        void lookupNode(Node *node);

        void clear();

    protected:
        bool tryDeleteSelf() override;

    private:
        std::map<UiElement *, SubscriptionHandle> _subscriptions{};

        Node *_engineNode = nullptr;
        UiCheckBox *_activeCheckBox = nullptr;
        UiTextBox *_nameTextBox = nullptr;
        std::vector<Component *> _trackedComponents{};
        std::vector<UiComponent *> _uiComponentElements{};

        void initialize();

        void resetElementsState();

        void onNodeActiveChanged(bool isActive) const;

        void onNodeNameChanged(const char *name) const;

        void setupUiComponent(UiComponent *uiComponentElement) const;

        void onUiComponentDeleted(std::type_index type);
    };
} // BreadEditor

#pragma once
#include <map>

#include "node.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiCheckBox.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiTextBox.h"
using namespace BreadEngine;

namespace BreadEditor {
    class NodeInspector final : public UiElement, IUiResizable
    {
    public:
        explicit NodeInspector(const std::string &id);

        explicit NodeInspector(const std::string &id, UiElement *parentElement);

        ~NodeInspector() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void lookupNode(Node *node);

        void clear();

    private:
        std::map<UiElement *, SubscriptionHandle> subscriptions{};

        Node *engineNode = nullptr;
        UiCheckBox *activeCheckBox = nullptr;
        UiTextBox *nameTextBox = nullptr;

        void initialize();

        void resetElementsState();

        void onNodeActiveChanged(bool isActive);

        void onNodeNameChanged(const char *name);
    };
} // BreadEditor

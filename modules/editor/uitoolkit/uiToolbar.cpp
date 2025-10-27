#include "uiToolbar.h"
#include "../windows/mainWindow/mainWindow.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor
{
    vector<BreadEngine::SubscriptionHandle> subscriptions{};

    UiToolbar::UiToolbar() = default;

    UiToolbar &UiToolbar::setup(const string &id, UiElement *parentElement, float height, const vector<string> &buttonNames)
    {
        float lastX = 0;
        for (const auto &buttonName: buttonNames)
        {
            constexpr float offset = 5;
            auto tag = id + buttonName;
            auto &button = UiPool::labelButtonPool.get().setup("toolbar" + tag, this, buttonName);
            auto position = Vector2{lastX + offset, 0};
            auto size = Vector2{50, height - offset};
            button.setBounds(position, size);
            button.setAnchor(UI_LEFT_TOP);

            lastX += offset + size.x;
            subscriptions.emplace_back(button.onClick.subscribe([this](const string &a) { this->onButtonClicked(a); }));
        }

        return dynamic_cast<UiToolbar &>(UiElement::setup(id, parentElement));
    }

    UiToolbar::~UiToolbar()
    {
        for (int i = static_cast<int>(subscriptions.size()) - 1; i > 0; i--)
        {
            const auto button = dynamic_cast<UiLabelButton *>(childs[i]);
            button->onClick.unsubscribe(subscriptions[i]);
        }

        subscriptions.clear();
    }

    void UiToolbar::draw(const float deltaTime)
    {
        GuiSetState(state);
        GuiPanel(bounds, nullptr);
        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiToolbar::update(const float deltaTime)
    {
        UiElement::update(deltaTime);
    }

    void UiToolbar::onButtonClicked(const string &buttonName)
    {
        TraceLog(LOG_INFO, "Button clicked: %s", buttonName.c_str());
    }

    void UiToolbar::deleteSelf()
    {
        UiPool::toolbarPool.release(*this);
    }
} // namespace BreadEditor

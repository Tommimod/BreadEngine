#include "uiToolbar.h"
#include "../windows/mainWindow.h"
#include "raygui.h"
#include "uiPool.h"
#include "raymath.h"

namespace BreadEditor {
    UiToolbar::UiToolbar() = default;

    UiToolbar &UiToolbar::setup(const string &id, UiElement *parentElement, const vector<string> &buttonNames, const bool isVisualAsLabel)
    {
        _isVisualAsLabel = isVisualAsLabel;
        replaceButtons(buttonNames);
        return dynamic_cast<UiToolbar &>(UiElement::setup(id, parentElement));
    }

    UiToolbar::~UiToolbar() = default;

    void UiToolbar::dispose()
    {
        onButtonPressed.unsubscribeAll();
        onButtonRequestedToRemove.unsubscribeAll();
        UiElement::dispose();
    }

    void UiToolbar::draw(const float deltaTime)
    {
        GuiPanel(_bounds, nullptr);
    }

    void UiToolbar::update(const float deltaTime)
    {
    }

    void UiToolbar::replaceButtons(const vector<string> &buttonNames)
    {
        destroyAllChilds();

        float lastX = 0;
        for (const auto &buttonName: buttonNames)
        {
            constexpr float offset = 5;
            auto tag = id + buttonName;
            const auto textWidth = fmax(50.0f, static_cast<float>(GuiGetTextWidth(buttonName.c_str())) * 1.2f);
            auto size = Vector2{static_cast<float>(textWidth), .0f};
            if (_isVisualAsLabel)
            {
                auto &button = UiPool::labelButtonPool.get().setup(id + tag, this, buttonName);
                auto position = Vector2{lastX + offset, 0};
                button.setSizePercentPermanent({-1, 1});
                button.setBounds(position, size);
                button.setTextAlignment(TEXT_ALIGN_CENTER);
                lastX += offset + size.x;
                button.onClick.subscribe([this](const UiLabelButton *a) { this->invokeButtonClicked(a); });
            }
            else
            {
                auto &button = UiPool::buttonPool.get().setup(id + tag, this, buttonName);
                auto position = Vector2{lastX + offset, 0};
                button.setSizePercentPermanent({-1, 1});
                button.setBounds(position, size);
                button.setTextAlignment(TEXT_ALIGN_LEFT);
                lastX += offset + size.x;
                button.onClick.subscribe([this](const UiButton *a) { this->invokeButtonClicked(a); });

                auto &closeButton = UiPool::labelButtonPool.get().setup(id + tag, &button, "X");
                closeButton.setAnchor(UI_RIGHT_CENTER);
                closeButton.setPivot({1,.5f});
                closeButton.setSize({10,10});
                closeButton.setPosition({-5, 0});
                closeButton.setTextSize(10);
                closeButton.onClick.subscribe([this](const UiLabelButton *a) { this->invokeButtonRequestedToRemove(a); });
            }
        }
    }

    void UiToolbar::invokeButtonClicked(const UiButton *button)
    {
        TraceLog(LOG_INFO, "Button clicked: %s", button->id.c_str());
        onButtonPressed.invoke(button->getIndex());
    }

    void UiToolbar::invokeButtonClicked(const UiLabelButton *button)
    {
        TraceLog(LOG_INFO, "Button clicked: %s", button->id.c_str());
        onButtonPressed.invoke(button->getIndex());
    }

    void UiToolbar::invokeButtonRequestedToRemove(const UiLabelButton *button)
    {
        onButtonRequestedToRemove.invoke(button->getParentElement()->getIndex());
    }

    bool UiToolbar::tryDeleteSelf()
    {
        UiPool::toolbarPool.release(*this);
        return true;
    }
} // namespace BreadEditor

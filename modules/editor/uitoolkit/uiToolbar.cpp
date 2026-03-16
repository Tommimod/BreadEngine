#include "uiToolbar.h"
#include "editor.h"
#include "editorStyle.h"
#include "../windows/mainWindow.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiToolbar::UiToolbar() = default;

    UiToolbar &UiToolbar::setup(const string_view &id, UiElement *parentElement, const vector<string_view> &buttonNames, const bool isVisualAsLabel)
    {
        _isVisualAsLabel = isVisualAsLabel;
        replaceButtons(buttonNames);
        return dynamic_cast<UiToolbar &>(UiElement::setup(id, parentElement));
    }

    UiToolbar::~UiToolbar() = default;

    void UiToolbar::dispose()
    {
        _buttons.clear();
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

    void UiToolbar::replaceButtons(const vector<string_view> &buttonNames)
    {
        for (const auto button: _buttons)
        {
            button->changeParent(&Editor::getInstance().mainWindow);
            button->getParentElement()->destroyChild(button);
        }

        _buttons.clear();

        auto lastX = 0.0f;
        auto index = 0;
        for (const auto &buttonName: buttonNames)
        {
            constexpr float offset = 5;
            const std::string_view tag = TextFormat("%s_%s_%d", id.c_str(), buttonName, index);
            index++;
            const auto textWidth = static_cast<float>(GuiGetTextWidth(buttonName.data())) * 1.7f;
            auto size = Vector2{static_cast<float>(textWidth), .0f};
            if (_isVisualAsLabel)
            {
                const auto button = &UiPool::labelButtonPool.get().setup(tag, this, buttonName.data());
                auto position = Vector2{lastX + offset, 0};
                button->setSizePercentPermanent({-1, 1});
                button->setBounds(position, size);
                button->setTextAlignment(TEXT_ALIGN_CENTER);
                button->setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
                lastX += offset + size.x;
                button->onClick.subscribe([this](UiLabelButton *a) { this->invokeButtonClicked(a); });
                _buttons.emplace_back(button);
            }
            else
            {
                auto button = &UiPool::buttonPool.get();
                button = &button->setup(tag, this, buttonName.data());
                auto position = Vector2{lastX + offset, 0};
                button->setSizePercentPermanent({-1, 1});
                button->setBounds(position, size);
                button->setTextAlignment(TEXT_ALIGN_LEFT);
                button->setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
                lastX += offset + size.x;
                button->onClick.subscribe([this](UiButton *a) { this->invokeButtonClicked(a); });
                _buttons.emplace_back(button);

                const auto closeButton = &UiPool::labelButtonPool.get().setup(TextFormat("%s_closeB", tag), button, "X");
                closeButton->setAnchor(UI_RIGHT_CENTER);
                closeButton->setPivot({1, .5f});
                closeButton->setSize({10, 10});
                closeButton->setPosition({-5, 0});
                closeButton->setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
                closeButton->onClick.subscribe([this](const UiLabelButton *a) { this->invokeButtonRequestedToRemove(a); });
            }
        }
    }

    void UiToolbar::invokeButtonClicked(UiButton *button)
    {
        TraceLog(LOG_INFO, "Button clicked: %s", button->id.c_str());
        onButtonPressed.invoke(button);
    }

    void UiToolbar::invokeButtonClicked(UiLabelButton *button)
    {
        TraceLog(LOG_INFO, "Button clicked: %s", button->id.c_str());
        onButtonPressed.invoke(button);
    }

    void UiToolbar::invokeButtonRequestedToRemove(const UiLabelButton *button)
    {
        TraceLog(LOG_INFO, "Tab closed: %s", button->getParentElement()->id.c_str());
        onButtonRequestedToRemove.invoke(button->getParentElement());
    }

    bool UiToolbar::tryDeleteSelf()
    {
        UiPool::toolbarPool.release(*this);
        return true;
    }
} // namespace BreadEditor

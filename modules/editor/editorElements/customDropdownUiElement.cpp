#include "customDropdownUiElement.h"

#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    CustomDropdownUiElement &CustomDropdownUiElement::setup(const std::string_view &id, UiElement *parentElement, const std::vector<std::string> &options)
    {
        UiElement::setup(id, parentElement);
        setOptions(options);
        return *this;
    }

    void CustomDropdownUiElement::awake()
    {
        _searchBox = &UiPool::textBoxPool.get().setup(id + "_searchBox", this, "Search:");
        _searchBox->setSizePercentPermanent({.65f, -1});
        _searchBox->setSize({-1, 20});
        _searchBox->setPosition({10, 2});
        _searchBox->setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));

        _content = &UiPool::scrollPanelPool.get().setup(id + "_content", this);
        _content->setSizePercentPermanent({.9f, 1}, {0, 30});
        _content->setPivot({.5, 0});
        _content->setAnchor(UI_CENTER_TOP);
        _content->setPosition({0, 30});
        updateContent();
    }

    void CustomDropdownUiElement::draw(float deltaTime)
    {
        GuiPanel(_bounds, nullptr);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !Engine::isCollisionPointRec(GetMousePosition(), _bounds))
        {
            std::string empty = "";
            onOptionSelected.invoke(empty);
        }
    }

    void CustomDropdownUiElement::dispose()
    {
        _searchBox = nullptr;
        _content = nullptr;
        _options.clear();
        onOptionSelected.unsubscribeAll();
        UiElement::dispose();
    }

    void CustomDropdownUiElement::setOptions(const std::vector<std::string> &options)
    {
        _options = options;
        if (_content == nullptr) return;
        updateContent();
    }

    void CustomDropdownUiElement::close()
    {
        isActive = false;
        _content->destroyAllChilds();
        onOptionSelected.unsubscribeAll();
    }

    void CustomDropdownUiElement::updateContent()
    {
        if (_options.empty()) return;

        UiLabelButton *lastButton = nullptr;
        auto prevPosition = Vector2{0, 1};
        for (const auto &option: _options)
        {
            lastButton = &UiPool::labelButtonPool.get().setup(id + "_button_" + option, _content, option);
            lastButton->setPivot({.5f, 0});
            lastButton->setAnchor(UI_CENTER_TOP);
            lastButton->setTextAlignment(TEXT_ALIGN_CENTER);
            lastButton->setPosition(prevPosition);
            prevPosition.y += 21;
            lastButton->setSizePercentPermanent({.9f, -1});
            lastButton->setSize({-1, 20});
            lastButton->onClick.subscribe([this](UiLabelButton *button)
            {
                if (_lastClickedButton != button)
                {
                    if (_lastClickedButton != nullptr)
                    {
                        _lastClickedButton->setState(STATE_NORMAL);
                    }

                    _lastClickedButton = button;
                    button->setState(STATE_PRESSED);
                }
                else
                {
                    onOptionSelected.invoke(_options[button->getIndex()]);
                }
            });
        }

        _content->calculateRectForScroll(lastButton);
    }
} // BreadEditor

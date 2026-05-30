#include "uiDropdown.h"
#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiDropdown::UiDropdown() = default;

    UiDropdown::~UiDropdown()
    {
        delete[] _optionsForGui;
    }

    UiDropdown &UiDropdown::setup(const std::string_view &id, const std::vector<std::string> &options, const bool isPermanent)
    {
        UiElement::setup(id);
        changeOptions(options);
        _isShouldBeDeleted = !isPermanent;
        _inOpenState = _isShouldBeDeleted;

        if (!_isShouldBeDeleted) enableOverlayLayer();
        return *this;
    }

    UiDropdown &UiDropdown::setup(const std::string_view &id, UiElement *parentElement, const std::vector<std::string> &options, const bool isPermanent)
    {
        UiElement::setup(id, parentElement);
        changeOptions(options);
        _isShouldBeDeleted = !isPermanent;
        _inOpenState = _isShouldBeDeleted;

        if (!_isShouldBeDeleted) enableOverlayLayer();
        return *this;
    }

    void UiDropdown::draw(const float deltaTime)
    {
        if (_options.empty())
        {
            getParentElement()->destroyChild(this);
            return;
        }

        EditorStyle::setDrowDownBoxTextAlignment(_textAlignment);
        if (GuiDropdownBox(_bounds, _optionsForGui, &_selectedOption, _inOpenState))
        {
            if (!isCollisionPointRec(GetMousePosition(), _bounds))
            {
                _selectedOption = -2;
            }

            onOptionSelected.invoke(_selectedOption + 1);
        }
    }

    void UiDropdown::update(const float deltaTime)
    {
        if (_isShouldBeDeleted && !isCollisionPointRec(GetMousePosition(), _bounds)
            && (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
        {
            getParentElement()->destroyChild(this);
        }
        else if (!_isShouldBeDeleted && _state != STATE_DISABLED)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Engine::isCollisionPointRec(GetMousePosition(), _bounds))
            {
                _inOpenState = true;
                enableOverlayLayer();
            }

            if (_inOpenState && !isCollisionPointRec(GetMousePosition(), _bounds))
            {
                _inOpenState = false;
                disableOverlayLayer();
            }
        }
    }

    void UiDropdown::dispose()
    {
        _options.clear();
        onOptionSelected.unsubscribeAll();
        _optionsForGui = nullptr;
        _selectedOption = 0;
        _elementsCount = 0;
        _inOpenState = true;
        _isShouldBeDeleted = true;
        UiElement::dispose();
    }

    void UiDropdown::changeOptions(const std::vector<std::string> &options)
    {
        _elementsCount = 0;
        _options.clear();
        const int size = static_cast<int>(options.size());
        for (const auto &option: options)
        {
            _elementsCount++;
            if (_elementsCount == size)
            {
                _options += option;
            }
            else
            {
                _options += option + ";";
            }
        }

        _optionsForGui = _options.c_str();
    }

    bool UiDropdown::tryDeleteSelf()
    {
        UiPool::dropdownPool.release(*this);
        return true;
    }

    bool UiDropdown::isCollisionPointRec(const Vector2 point, Rectangle &bounds)
    {
        const auto elementsSize = (bounds.height + static_cast<float>(GuiGetStyle(DROPDOWNBOX, DROPDOWN_ITEMS_SPACING))) * static_cast<float>(_elementsCount);
        return point.x >= bounds.x && point.x <= bounds.x + bounds.width &&
               point.y >= bounds.y && point.y <= bounds.y + bounds.height + elementsSize;
    }
} // BreadEditor

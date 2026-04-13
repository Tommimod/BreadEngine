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
        changeOptions(options);
        UiElement::setup(id);
        _isShouldBeDeleted = !isPermanent;
        _inOpenState = _isShouldBeDeleted;

        if (!_isShouldBeDeleted) enableOverlayLayer();
        return *this;
    }

    UiDropdown &UiDropdown::setup(const std::string_view &id, UiElement *parentElement, const std::vector<std::string> &options, const bool isPermanent)
    {
        changeOptions(options);
        UiElement::setup(id, parentElement);
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
            if (!isCollisionPointRec(GetMousePosition()))
            {
                _selectedOption = -2;
            }

            onOptionSelected.invoke(_selectedOption + 1);
        }
    }

    void UiDropdown::update(const float deltaTime)
    {
        if (_isShouldBeDeleted && !isCollisionPointRec(GetMousePosition())
            && (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
        {
            getParentElement()->destroyChild(this);
        }
        else if (!_isShouldBeDeleted)
        {
            if (Engine::isCollisionPointRec(GetMousePosition(), _bounds))
            {
                _inOpenState = true;
                enableOverlayLayer();
            }

            if (_inOpenState && !isCollisionPointRec(GetMousePosition()))
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

    bool UiDropdown::isCollisionPointRec(const Vector2 point) const
    {
        const auto elementsSize = (_bounds.height + static_cast<float>(GuiGetStyle(DROPDOWNBOX, DROPDOWN_ITEMS_SPACING))) * static_cast<float>(_elementsCount);
        return point.x >= _bounds.x && point.x <= _bounds.x + _bounds.width &&
               point.y >= _bounds.y && point.y <= _bounds.y + _bounds.height + elementsSize;
    }
} // BreadEditor

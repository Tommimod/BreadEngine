#include "uiDropdown.h"
#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiDropdown::UiDropdown() = default;

    UiDropdown::~UiDropdown()
    {
        delete[] _optionsForGui;
    }

    UiDropdown &UiDropdown::setup(const std::string &id, const std::vector<std::string> &options, const bool isPermanent)
    {
        changeOptions(options);
        _isEditMode = !isPermanent;
        UiElement::setup(id);
        return *this;
    }

    UiDropdown &UiDropdown::setup(const std::string &id, UiElement *parentElement, const std::vector<std::string> &options, const bool isPermanent)
    {
        changeOptions(options);
        _isEditMode = !isPermanent;
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiDropdown::draw(const float deltaTime)
    {
        if (!isActive) return;

        if (_elementsCount == 0)
        {
            changeParent(nullptr);
            tryDeleteSelf();
            return;
        }

        GuiSetState(_state);
        GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, _textAlignment);
        if (GuiDropdownBox(_bounds, _optionsForGui, &_selectedOption, _isEditMode))
        {
            if (!isCollisionPointRec(GetMousePosition()))
            {
                _selectedOption = -1;
            }

            onValueChanged.invoke(_selectedOption);
        }

        UiElement::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void UiDropdown::update(const float deltaTime)
    {
        if (!isActive) return;

        UiElement::update(deltaTime);
    }

    void UiDropdown::dispose()
    {
        _options.clear();
        onValueChanged.unsubscribeAll();
        _optionsForGui = nullptr;
        _selectedOption = 0;
        _elementsCount = 0;
        UiElement::dispose();
    }

    void UiDropdown::changeOptions(const std::vector<std::string> &options)
    {
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

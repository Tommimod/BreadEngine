#include "IUiOptionsOwner.h"
#include "editor.h"
#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    void IUiOptionsOwner::updateOptionsOwner()
    {
        if (_element == nullptr) return;
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && Engine::isCollisionPointRec(GetMousePosition(), _element->getBounds()))
        {
            showOptionsDropdown();
        }
    }

    void IUiOptionsOwner::showOptionsDropdown()
    {
        if (_options.empty()) return;
        const auto root = &Editor::getInstance().mainWindow;
        _dropdown = &UiPool::dropdownPool.get().setup(TextFormat("%s_optionsDropdown", _element->id), root, _options, false);
        _dropdown->setAnchor(UI_LEFT_TOP);
        _dropdown->setPivot({0, 0});
        _dropdown->setSize({80, 15});
        _dropdown->setPosition(GetMousePosition());
        _dropdown->setTextAlignment(TEXT_ALIGN_LEFT);
        _dropdown->setOnOverlayLayer();
        _dropdown->onOptionSelected.subscribe([this](const int value)
        {
            _element->destroyChild(_dropdown);
            handleSelectedOption(value);
        });
    }
} // BreadEditor

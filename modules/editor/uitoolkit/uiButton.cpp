#include "engine.h"
#include "UiButton.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiButton::UiButton() = default;

    UiButton::~UiButton() = default;

    UiButton &UiButton::setup(const std::string &id, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiButton &>(UiElement::setup(id));
    }

    UiButton &UiButton::setup(const std::string &id, UiElement *parentElement, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiButton &>(UiElement::setup(id, parentElement));
    }

    void UiButton::dispose()
    {
        onClick.unsubscribeAll();
        UiElement::dispose();
    }

    void UiButton::draw(const float deltaTime)
    {
        GuiSetStyle(DEFAULT, TEXT_SIZE, _textSize);
        GuiSetStyle(BUTTON, TEXT_ALIGNMENT, _textAlignment);
        if (GuiButton(_bounds, _text.c_str()))
        {
            const auto mousePos = GetMousePosition();
            auto isNeedIgnore = !Engine::isCollisionPointRec(mousePos, _parent->getBounds()) && !_canClickOutside;
            if (const auto childsCount = getChildCount(); !isNeedIgnore && childsCount > 0)
            {
                for (const auto child : getAllChilds())
                {
                    if (Engine::isCollisionPointRec(mousePos, child->getBounds()))
                    {
                        isNeedIgnore = true;
                    }
                }

                if (!isNeedIgnore)
                {
                    onClick.invoke(this);
                }
            }
            else if (!isNeedIgnore)
            {
                onClick.invoke(this);
            }
        }
    }

    void UiButton::update(const float deltaTime)
    {
    }

    bool UiButton::tryDeleteSelf()
    {
        UiPool::buttonPool.release(*this);
        return true;
    }
} // namespace BreadEditor

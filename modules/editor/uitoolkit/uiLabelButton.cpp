#include "uiLabelButton.h"
#include "engine.h"
#include "raygui.h"
#include "uiPool.h"

namespace BreadEditor {
    UiLabelButton::UiLabelButton() = default;

    UiLabelButton::~UiLabelButton() = default;

    UiLabelButton &UiLabelButton::setup(const std::string &id, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id));
    }

    UiLabelButton &UiLabelButton::setup(const std::string &id, UiElement *parentElement, const std::string &newText)
    {
        this->_text = newText;
        return dynamic_cast<UiLabelButton &>(UiElement::setup(id, parentElement));
    }

    void UiLabelButton::dispose()
    {
        onClick.unsubscribeAll();
        _text.clear();
        _textAlignment = TEXT_ALIGN_LEFT;
        _textSize = 10;
        UiElement::dispose();
    }

    void UiLabelButton::draw(const float deltaTime)
    {
        GuiSetStyle(DEFAULT, TEXT_SIZE, _textSize);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, _textAlignment);
        if (GuiLabelButton(_bounds, _text.c_str()))
        {
            if (const auto childsCount = getChildCount(); childsCount > 0)
            {
                auto isNeedIgnore = false;
                for (const auto child: getAllChilds())
                {
                    if (Engine::isCollisionPointRec(GetMousePosition(), child->getBounds()))
                    {
                        isNeedIgnore = true;
                    }
                }

                if (!isNeedIgnore)
                {
                    onClick.invoke(this);
                }
            }
            else
            {
                onClick.invoke(this);
            }
        }
    }

    void UiLabelButton::update(const float deltaTime)
    {
    }

    bool UiLabelButton::tryDeleteSelf()
    {
        UiPool::labelButtonPool.release(*this);
        return true;
    }
} // namespace BreadEditor

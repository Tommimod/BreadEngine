#include "messageUiElement.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    MessageUiElement::MessageUiElement()
    {
    }

    MessageUiElement::~MessageUiElement()
    {
    }

    MessageUiElement &MessageUiElement::setup(const std::string_view &id, UiElement *parentElement, const std::string_view &messageText)
    {
        _messageText = messageText;
        UiElement::setup(id, parentElement);

        setTextAlignment(TEXT_ALIGN_RIGHT);
        setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
        setText(_messageText);
        return *this;
    }

    void MessageUiElement::draw(const float deltaTime)
    {
        UiButton::draw(deltaTime);
    }

    void MessageUiElement::dispose()
    {
        _messageText.clear();
        UiButton::dispose();
    }

    bool MessageUiElement::tryDeleteSelf()
    {
        UiPool::messageUiElementPool.release(*this);
        return true;
    }
} // BreadEditor

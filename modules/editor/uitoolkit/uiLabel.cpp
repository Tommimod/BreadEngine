#include "uiLabel.h"

#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiLabel::~UiLabel() = default;

    void UiLabel::dispose()
    {
        _text.clear();
        _textAlignment = TEXT_ALIGN_LEFT;
        _verticalAlignment = TEXT_ALIGN_MIDDLE;
        _wrapMode = TEXT_WRAP_NONE;
        _textSize = static_cast<int>(EditorStyle::FontSize::Medium);
        UiElement::dispose();
    }

    UiLabel &UiLabel::setup(const std::string_view &id, UiElement *parentElement, const std::string &text)
    {
        UiElement::setup(id, parentElement);
        _text = text;
        return *this;
    }

    void UiLabel::draw(const float deltaTime)
    {
        EditorStyle::setFontSize(_textSize);
        EditorStyle::setLabelTextAlignment(_textAlignment);
        EditorStyle::setGlobalTextVerticalAlignment(_verticalAlignment);
        EditorStyle::setGlobalTextWrapMode(_wrapMode);
        GuiLabel(_bounds, _text.c_str());
    }

    void UiLabel::update(const float deltaTime)
    {
    }

    bool UiLabel::tryDeleteSelf()
    {
        UiPool::labelPool.release(*this);
        return true;
    }
} // BreadEditor

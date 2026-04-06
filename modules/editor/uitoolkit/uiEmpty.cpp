#include "uiEmpty.h"

#include "uiPool.h"

namespace BreadEditor {
    UiEmpty &UiEmpty::setup(const std::string_view &id, UiElement *parentElement)
    {
        UiElement::setup(id, parentElement);
        return *this;
    }

    bool UiEmpty::tryDeleteSelf()
    {
        UiPool::emptyElementPool.release(*this);
        return true;
    }
} // BreadEditor

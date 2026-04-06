#pragma once
#include "uiElement.h"

namespace BreadEditor {
    class UiEmpty : public UiElement
    {
    public:
        UiEmpty &setup(const std::string_view &id, UiElement *parentElement);
    protected:
        bool tryDeleteSelf() override;
    };
} // BreadEditor
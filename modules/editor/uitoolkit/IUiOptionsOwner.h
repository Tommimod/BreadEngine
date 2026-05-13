#pragma once
#include <vector>
#include "uiDropdown.h"
#include "uiElement.h"

namespace BreadEditor {
    class IUiOptionsOwner
    {
    public:
        virtual ~IUiOptionsOwner() = default;

    protected:
        void initOptionsOwner(UiElement *element)
        {
            _element = element;
        }

        void disposeOptionsOwner()
        {
            _element = nullptr;
            _dropdown = nullptr;
            _options.clear();
        }

        void updateOptionsOwner();

        virtual std::vector<std::string> getOptions() = 0;

        virtual void handleSelectedOption(int index) = 0;

    private:
        UiDropdown *_dropdown = nullptr;
        UiElement *_element = nullptr;
        std::vector<std::string> _options;

        void getOptionsInternal();

        void showOptionsDropdown();
    };
} // BreadEditor

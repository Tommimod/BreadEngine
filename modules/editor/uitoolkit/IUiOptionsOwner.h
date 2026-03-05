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
        void initializeOptionsOwner(UiElement *element, std::vector<std::string> options)
        {
            _element = element;
            setOptions(std::move(options));
        }

        void disposeOptionsOwner()
        {
            _element = nullptr;
            _dropdown = nullptr;
            _options.clear();
        }

        void setOptions(std::vector<std::string> options)
        {
            _options = std::move(options);
        }

        void updateOptionsOwner();

        virtual void handleSelectedOption(int index) = 0;

    private:
        UiDropdown *_dropdown = nullptr;
        UiElement *_element = nullptr;
        std::vector<std::string> _options;

        void showOptionsDropdown();
    };
} // BreadEditor

#pragma once
#include "action.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiDropdown final : public UiElement
    {
    public:
        Action<int> onValueChanged;

        UiDropdown();

        ~UiDropdown() override;

        UiDropdown &setup(const std::string &id, const std::vector<std::string> &options, bool isPermanent);

        UiDropdown &setup(const std::string &id, UiElement *parentElement, const std::vector<std::string> &options, bool isPermanent);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setTextAlignment(const GuiTextAlignment alignment) { _textAlignment = alignment; }

        void changeOptions(const std::vector<std::string> &options);

    protected:
        bool tryDeleteSelf() override;

    private:
        GuiTextAlignment _textAlignment = TEXT_ALIGN_CENTER;
        std::string _options;
        const char *_optionsForGui{};
        int _selectedOption = 0;
        int _elementsCount = 0;
        bool _isEditMode = true;

        bool isCollisionPointRec(Vector2 point) const;
    };
} // BreadEditor

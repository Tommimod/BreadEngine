#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiLabelButton final : public UiElement
    {
    public:
        BreadEngine::Action<std::string> onClick;

        UiLabelButton();

        UiLabelButton &setup(const std::string &id, const std::string &newText);

        UiLabelButton &setup(const std::string &id, UiElement *parentElement, const std::string &newText);

        ~UiLabelButton() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
    };
} // namespace BreadEditor

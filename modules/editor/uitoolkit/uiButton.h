#pragma once
#include "action.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiButton final : public UiElement
    {
    public:
        Action<UiButton *> onClick;

        UiButton();

        UiButton &setup(const std::string &id, const std::string &newText);

        UiButton &setup(const std::string &id, UiElement *parentElement, const std::string &newText);

        ~UiButton() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _text;
    };
} // namespace BreadEditor

#pragma once
#include "action.h"
#include "uiElement.h"
#include "uiNumberBox.h"
#include "uiInspector.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiVector2D final : public UiElement
    {
    public:
        Action<Vector2> onChanged;

        UiVector2D() = default;

        ~UiVector2D() override = default;

        void dispose() override;

        UiVector2D *setup(const std::string &id, UiElement *parentElement, Vector2 initialValue);

        UiVector2D *setup(const std::string &id, UiElement *parentElement, Vector2 initialValue, std::string_view xName, std::string_view yName);

        UiVector2D *setup(const std::string &id, UiElement *parentElement, std::function<Vector2()> getFunc);

        UiVector2D *setup(const std::string &id, UiElement *parentElement, std::function<Vector2()> getFunc, std::string_view xName, std::string_view yName);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        Vector2 _value{};
        std::array<UiNumberBox *, 2> _fields{};
        std::array<std::string, 2> _names{};
        std::function<Vector2()> _getFunc;

        void createFields();

        void setValue(UiNumberBox *numberBox, float value);
    };
} // BreadEditor

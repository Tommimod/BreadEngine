#pragma once
#include "action.h"
#include "uiElement.h"
#include "uiNumberBox.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiVector4D final : public UiElement
    {
    public:
        Action<Vector4> onChanged;

        UiVector4D() = default;

        ~UiVector4D() override;

        UiVector4D *setup(const std::string &id, UiElement *parentElement, Vector4 initialValue);

        UiVector4D *setup(const std::string &id, UiElement *parentElement, Vector4 initialValue, std::string_view xName, std::string_view yName, std::string_view zName, std::string_view wName);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        Vector4 _value{};
        std::array<UiNumberBox *, 4> _fields{};
        std::array<std::string, 4> _names{};

        void createFields();

        void setValue(UiNumberBox *numberBox, float value);
    };
} // BreadEditor

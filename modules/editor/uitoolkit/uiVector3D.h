#pragma once
#include "action.h"
#include "uiElement.h"
#include "uiTextBox.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiVector3D final : public UiElement
    {
    public:
        Action<Vector3> onChanged;

        UiVector3D() = default;

        ~UiVector3D() override;

        UiVector3D *setup(const std::string &id, UiElement *parentElement, Vector3 initialValue);

        UiVector3D *setup(const std::string &id, UiElement *parentElement, Vector3 initialValue, std::string_view xName, std::string_view yName, std::string_view zName);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        bool tryDeleteSelf() override;

    private:
        Vector3 _value{};
        std::array<UiTextBox *, 3> _fields{};
        std::array<std::string, 3> _names{};

        void createFields();

        void setValue(UiTextBox *textBox, float value);
    };
} // BreadEditor

#pragma once
#include <variant>
#include "action.h"
#include "editorStyle.h"
#include "uiElement.h"
using namespace BreadEngine;

namespace BreadEditor {
    class UiNumberBox final : public UiElement
    {
    public:
        Action<float> onValueChanged;
        Action<float, UiNumberBox *> onValueChangedWithSender;

        UiNumberBox();

        ~UiNumberBox() override;

        void dispose() override;

        UiNumberBox &setup(const std::string &id, const std::string &label, float defaultValue, int defaultTextSize = 14, bool defaultEditMode = false);

        UiNumberBox &setup(const std::string &id, UiElement *parentElement, const std::string &label, float defaultValue, int defaultTextSize = 14, bool defaultEditMode = false);

        UiNumberBox &setup(const std::string &id, const std::string &label, int defaultValue, int defaultTextSize = 14, bool defaultEditMode = false);

        UiNumberBox &setup(const std::string &id, UiElement *parentElement, const std::string &label, int defaultValue, int defaultTextSize = 14, bool defaultEditMode = false);

        UiNumberBox &setup(const std::string &id, UiElement *parentElement, const std::string &label, std::function<std::variant<int, float, long>()> getFunc, int defaultTextSize = 16, bool defaultEditMode = false);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setValue(int value);

        void setValue(float value);

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _editMode = false;
        bool _intMode = false;
        int _intValue = 0;
        int _textSize = static_cast<int>(EditorStyle::FontSize::Medium);
        float _floatValue = 0;
        char _valueText[32] = {};
        std::string _label;
        std::string _floatLabel;
        std::function<std::variant<int, float, long>()> _getFunc;
    };
} // BreadEditor

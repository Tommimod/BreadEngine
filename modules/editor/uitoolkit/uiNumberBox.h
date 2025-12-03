#pragma once
#include "action.h"
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

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void setValue(float value);

    protected:
        bool tryDeleteSelf() override;

    private:
        bool _editMode = false;
        bool _intMode = false;
        float _floatValue = 0;
        int _intValue = 0;
        int _textSize = 0;
        const char *_label = nullptr;
        char _valueText[32] = {};
        std::string _floatLabel;
    };
} // BreadEditor

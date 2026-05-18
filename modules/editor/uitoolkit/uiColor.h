#pragma once
#include "action.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiColor final : public UiElement
    {
    public:
        BreadEngine::Action<const Color &> onSelectorRequested;
        BreadEngine::Action<Color> onValueChanged;

        UiColor();

        UiColor &setup(const std::string_view &id, Color color);

        UiColor &setup(const std::string_view &id, UiElement *parentElement, Color color);

        UiColor &setup(const std::string_view &id, UiElement *parentElement, const std::function<Color()> &getFunc);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setColor(Color color, bool withNotification);

        void setUpdateState(bool state);

    protected:
        bool tryDeleteSelf() override;

    private:
        Color _value = WHITE;
        Color *_internalValue = nullptr;
        std::function<Color()> _getFunc;
        bool _updateState = true;
    };
} // BreadEditor

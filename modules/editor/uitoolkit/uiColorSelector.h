#pragma once
#include "inspectorObject.h"
#include "uiColor.h"
#include "uiElement.h"

namespace BreadEditor {
    class UiColorSelector final : public UiElement
    {
    public:
        UiColorSelector &setup(const std::string_view &id, Color color);

        UiColorSelector &setup(const std::string_view &id, UiElement *parentElement, Color color);

        UiColorSelector &setup(const std::string_view &id, UiElement *parentElement, Color color, UiColor *uiColorElement);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        UiColor *_uiColorElement = nullptr;
        Color _color = WHITE;
        Vector4 _color4 = {};
        Color *_internalColor = nullptr;
        Vector4 *_internalValue4 = nullptr;
        float *_internalAlpha = nullptr;
    };
} // BreadEditor

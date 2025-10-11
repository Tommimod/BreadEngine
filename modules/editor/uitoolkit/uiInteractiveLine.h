#pragma once
#include "uiElement.h"

namespace BreadEditor {
    class UiInteractiveLine : public UiElement
    {
    public:
        typedef enum InteractiveLineType
        {
            Click,
            Drag
        } InteractiveLineType;

        typedef struct Settings
        {
            InteractiveLineType type;
            Vector2 start;
            Vector2 end;
            Color color;
            float thickness;
        } Settings;

        UiInteractiveLine();

        UiInteractiveLine &setup(const std::string &id, Settings newSettings);

        UiInteractiveLine &setup(const std::string &id, UiElement *parentElement, Settings newSettings);

        ~UiInteractiveLine() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

    protected:
        void deleteSelf() override;

    private:
        Settings settings;
    };
} // BreadEditor

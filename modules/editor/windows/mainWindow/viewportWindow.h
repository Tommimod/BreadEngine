#pragma once
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class ViewportWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit ViewportWindow(const std::string &id);

        explicit ViewportWindow(const std::string &id, UiElement *parentElement);

        ~ViewportWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        const char *getTitle() override { return _title; }

        [[nodiscard]] bool isMouseOver() const;

        [[nodiscard]] Vector2 getMousePosition() const;

        [[nodiscard]] static Ray getMouseRay(Vector2 virtualMouse, Camera3D camera, int width, int height);

        [[nodiscard]] Rectangle getViewportSize() const;

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();
        Vector2 _mousePosition{0, 0};
    };
} // BreadEditor

#pragma once
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiPanel.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class ViewportWindow final : public UiWindow
    {
    public:
        enum ViewportMode
        {
            Scene,
            Game
        };

        static std::string Id;

        explicit ViewportWindow(const std::string_view &id);

        explicit ViewportWindow(const std::string_view &id, UiElement *parentElement);

        ~ViewportWindow() override;

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void onFrameEnd(float deltaTime) override;

        void dispose() override;

        const char *getTitle() override { return _title; }

        [[nodiscard]] bool isMouseOver() const;

        /// Where the pointer sits in the viewport's own pixels. Not clamped to it: what is
        /// under the pointer is isMouseOver()'s answer to give, and a drag that began inside
        /// still has to be followed once it leaves.
        [[nodiscard]] Vector2 getMousePosition() const;

        /// The ray through the pointer, in world space. Unprojected with the matrix the scene
        /// was actually drawn through, so what it reaches is what is on screen.
        [[nodiscard]] Ray getMouseRay() const;

        [[nodiscard]] Rectangle getViewportSize() const;

        [[nodiscard]] ViewportMode getMode() const { return _mode; }

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        UiPanel *_warningPanel = nullptr;
        Vector2 _mousePosition{0, 0};
        const char *_title = Id.c_str();
        ViewportMode _mode = Scene;

        void initializePanel() override;

        void cleanupPanel() override;
    };
} // BreadEditor

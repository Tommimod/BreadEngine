#pragma once
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiElement.h"

namespace BreadEditor {
    class UiWindow : public UiElement, public IUiResizable
    {
    public:
        explicit UiWindow(const std::string_view &id);

        explicit UiWindow(const std::string_view &id, UiElement *parentElement);

        ~UiWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void onFrameEnd(float deltaTime) override;

        void dispose() override;

        virtual const char *getTitle() { return ""; }

        void close();

    protected:
        Vector2 _scrollPos = {0.0f, 0.0f};
        Rectangle _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        Vector2 _contentSize{0, 0};

        virtual void initialize();

        virtual void subscribe();

        virtual void unsubscribe();

        bool tryDeleteSelf() override;

        virtual void updateScrollView();

        void calculateRectForScroll(UiElement *element);
    };
} // BreadEditor

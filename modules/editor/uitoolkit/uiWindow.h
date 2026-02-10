#pragma once
#include "uiButton.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiElement.h"

namespace BreadEditor {
    class UiWindow : public UiElement, public IUiResizable
    {
    public:
        explicit UiWindow(const std::string &id);

        explicit UiWindow(const std::string &id, UiElement *parentElement);

        ~UiWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void onFrameEnd(float deltaTime) override;

        void dispose() override;

        void setIsCloseable(const bool value) const { _closeButton.isActive = value; }

        virtual const char *getTitle() = 0;

    protected:
        Vector2 _scrollPos = {0.0f, 0.0f};
        Rectangle _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        Vector2 _contentSize{0, 0};
        UiButton &_closeButton;

        virtual void initialize();

        virtual void subscribe();

        virtual void unsubscribe();

        bool tryDeleteSelf() override;

        virtual void updateScrollView();

        void calculateRectForScroll(UiElement *element);
    };
} // BreadEditor

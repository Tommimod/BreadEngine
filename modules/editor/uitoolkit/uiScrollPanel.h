#pragma once
#include "uiElement.h"

namespace BreadEditor {
    class UiScrollPanel : public UiElement
    {
    public:
        UiScrollPanel();

        UiScrollPanel &setup(const std::string_view &id);

        UiScrollPanel &setup(const std::string_view &id, UiElement *parentElement);

        ~UiScrollPanel() override;

        void dispose() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void onFrameEnd(float deltaTime) override;

    protected:
        Rectangle _contentView = {0.0f, 0.0f, 0.0f, 0.0f};
        Rectangle _scrollView = {0.0f, 0.0f, 0.0f, 0.0f};
        Vector2 _scrollPos = {0.0f, 0.0f};
        Vector2 _contentSize{0, 0};
        const char *_title = nullptr;

        bool tryDeleteSelf() override;

        virtual void updateScrollView();

        void calculateRectForScroll(UiElement *element);
    };
} // BreadEditor

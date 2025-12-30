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

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = Id.c_str();
    };
} // BreadEditor

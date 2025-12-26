#pragma once
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    class AssetsWindow final : public UiWindow
    {
    public:
        static std::string Id;

        explicit AssetsWindow(const std::string &id);

        explicit AssetsWindow(const std::string &id, UiElement *parentElement);

        ~AssetsWindow() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        void subscribe() override;

        void unsubscribe() override;

    private:
        const char *_title = "Assets Window";
    };
} // BreadEditor

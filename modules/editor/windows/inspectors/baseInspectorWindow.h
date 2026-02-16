#pragma once
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiWindow.h"

namespace BreadEditor {
    template<typename T>
    class BaseInspectorWindow : public UiWindow
    {
    public:
        static std::string Id;

        explicit BaseInspectorWindow(const std::string &id);

        explicit BaseInspectorWindow(const std::string &id, UiElement *parentElement);

        ~BaseInspectorWindow() override = default;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override = 0;

        virtual void lookupObject(T *object) = 0;

        virtual void clear() = 0;

        [[nodiscard]] const char *getTitle() override { return _title; }

    private:
        const char *_title = Id.c_str();
    };
} // BreadEditor

#pragma once
#include "uiScrollPanel.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiElement.h"

namespace BreadEditor {
    class UiWindow : public UiScrollPanel, public IUiResizable
    {
    public:
        explicit UiWindow(const std::string_view &id);

        explicit UiWindow(const std::string_view &id, UiElement *parentElement);

        ~UiWindow() override;

        void update(float deltaTime) override;

        void dispose() override;

        virtual const char *getTitle() { return ""; }

        void close();

        void awake() override;

    protected:
        virtual void subscribe();

        virtual void unsubscribe();

        bool tryDeleteSelf() override;
    };
} // BreadEditor

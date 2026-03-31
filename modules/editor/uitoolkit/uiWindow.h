#pragma once
#include "action.h"
#include "uiScrollPanel.h"
#include "uitoolkit/IUiResizable.h"
#include "uitoolkit/uiElement.h"

namespace BreadEditor {
    class UiWindow : public UiElement, public IUiResizable
    {
    public:
        BreadEngine::Action<UiWindow *> onClose;

        explicit UiWindow(const std::string_view &id);

        explicit UiWindow(const std::string_view &id, UiElement *parentElement);

        ~UiWindow() override;

        void open() const;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        virtual const char *getTitle() { return ""; }

        void close();

    protected:
        UiScrollPanel *_content = nullptr;

        virtual void subscribe();

        virtual void unsubscribe();

        bool tryDeleteSelf() override;
    };
} // BreadEditor

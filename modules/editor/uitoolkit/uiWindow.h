#pragma once
#include "action.h"
#include "uiEmpty.h"
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

        void open();

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        virtual const char *getTitle() { return ""; }

        void close();

    protected:
        UiScrollPanel *_content = nullptr;
        UiEmpty *_topPanel = nullptr;

        virtual void subscribe();

        virtual void unsubscribe();

        bool tryDeleteSelf() override;

        [[nodiscard]] UiEmpty *getWindowPanel() const;

    private:
        void initializeWindow();

        virtual void initializePanel() = 0;

        virtual void cleanupPanel() = 0;
    };
} // BreadEditor

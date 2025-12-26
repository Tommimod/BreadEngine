#include "assetsWindow.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "mainWindowAssetsWindow";

    AssetsWindow::AssetsWindow(const std::string &id) : UiWindow(id)
    {
        setup(id);
        subscribe();
    }

    AssetsWindow::AssetsWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        setup(id, parentElement);
        subscribe();
    }

    AssetsWindow::~AssetsWindow()
    {
    }

    void AssetsWindow::draw(float deltaTime)
    {
        GuiSetState(_state);
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
        GuiSetState(STATE_NORMAL);
    }

    void AssetsWindow::update(float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void AssetsWindow::dispose()
    {
        UiWindow::dispose();
    }

    void AssetsWindow::subscribe()
    {
        UiWindow::subscribe();
    }

    void AssetsWindow::unsubscribe()
    {
        UiWindow::unsubscribe();
    }
} // BreadEditor

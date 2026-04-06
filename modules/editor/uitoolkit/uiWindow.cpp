#include "uiWindow.h"
#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string_view &id)
    {
        setup(id);
        initializeWindow();
    }

    UiWindow::UiWindow(const std::string_view &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        initializeWindow();
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::open()
    {
        Editor::getInstance().getEditorModel().getWindowsModel().removeWindowFromAllowList(id);
        initializePanel();
    }

    void UiWindow::draw(const float deltaTime)
    {
        EditorStyle::setFontSize(EditorStyle::FontSize::MediumLarge);
        if (GuiWindowBox(_bounds, id.c_str()))
        {
            close();
        }
    }

    void UiWindow::update(const float deltaTime)
    {
        updateResizable(*this);
    }

    void UiWindow::dispose()
    {
        _topPanel = nullptr;
        _content = nullptr;
        onClose.unsubscribeAll();
        UiWindow::unsubscribe();
        setVerticalResized(false);
        setHorizontalResized(false);
    }

    void UiWindow::close()
    {
        if (_topPanel != nullptr)
        {
            cleanupPanel();
            _topPanel = nullptr;
        }

        Editor::getInstance().getEditorModel().getWindowsModel().addWindowToAllowList(id);
        onClose.invoke(this);
    }

    void UiWindow::subscribe()
    {
    }

    void UiWindow::unsubscribe()
    {
    }

    bool UiWindow::tryDeleteSelf()
    {
        return false;
    }

    UiEmpty *UiWindow::getWindowPanel() const
    {
        return _topPanel;
    }

    void UiWindow::initializeWindow()
    {
        _content = &UiPool::scrollPanelPool.get().setup(id + "_content", this);
        _content->setSizePercentPermanent({1, .995f});
        _content->setPosition({0, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT});

        EditorStyle::setFontSize(EditorStyle::FontSize::MediumLarge);
        _topPanel = &UiPool::emptyElementPool.get().setup(id + "_topPanel", this);
        _topPanel->setAnchor(UI_RIGHT_TOP);
        _topPanel->setPivot({1, 0});
        _topPanel->setSizePercentPermanent({1, -1}, {static_cast<float>(GuiGetTextWidth(id.c_str())) + 45, 0});
        _topPanel->setSize({-1, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT});
        _topPanel->setPosition({-30, 0});
    }

    void UiWindow::initializePanel()
    {
    }

    void UiWindow::cleanupPanel()
    {
    }
} // BreadEditor

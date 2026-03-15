#include "uiWindow.h"
#include "editor.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string &id)
    {
        setup(id);
        UiWindow::initialize();
    }

    UiWindow::UiWindow(const std::string &id, UiElement *parentElement)
    {
        setup(id, parentElement);
        UiWindow::initialize();
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::draw(const float deltaTime)
    {
        BeginScissorMode(static_cast<int>(_scrollView.x), static_cast<int>(_scrollView.y), static_cast<int>(_scrollView.width), static_cast<int>(_scrollView.height));
    }

    void UiWindow::update(const float deltaTime)
    {
        updateResizable(*this);
        updateScrollView();
        setScrollOffset(_scrollPos);
    }

    void UiWindow::onFrameEnd(const float deltaTime)
    {
        EndScissorMode();
    }

    void UiWindow::dispose()
    {
        UiWindow::unsubscribe();
        UiElement::dispose();
        setVerticalResized(false);
        setHorizontalResized(false);
        _isAwake = false;
    }

    void UiWindow::close()
    {
        _parent->destroyChild(this);
        getRootElement()->setDirty();
        Editor::getInstance().getEditorModel().getWindowsModel()->addWindowToAllowList(id);
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

    void UiWindow::updateScrollView()
    {
        _contentView.x = getBounds().x;
        _contentView.y = getBounds().y;
        _contentView.height = _contentSize.y;
        _contentView.width = _contentSize.x;
        if (_contentView.height < getBounds().height)
        {
            _contentView.height = getBounds().height - static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) - 15;
        }

        if (_contentView.width < getBounds().width)
        {
            _contentView.width = getBounds().width - static_cast<float>(GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)) - static_cast<float>(GuiGetStyle(DEFAULT, BORDER_WIDTH)) - 1;
        }
    }

    void UiWindow::initialize()
    {
        Editor::getInstance().getEditorModel().getWindowsModel()->removeWindowFromAllowList(id);
    }

    void UiWindow::calculateRectForScroll(UiElement *element)
    {
        _contentSize = {0, 0};

        const auto &elementPosition = element->getPosition();
        const auto &elementSize = element->getSize();
        const auto size = Vector2(elementPosition.x + elementSize.x, elementPosition.y + elementSize.y);
        if (_contentSize.x == 0)
        {
            _contentSize.x = size.x;
        }
        else
        {
            if (_contentSize.x < size.x) _contentSize.x = size.x;
        }

        if (_contentSize.y == 0)
        {
            _contentSize.y = size.y;
        }
        else
        {
            if (_contentSize.y < size.y) _contentSize.y = size.y;
        }

        setDirty();
    }
} // BreadEditor

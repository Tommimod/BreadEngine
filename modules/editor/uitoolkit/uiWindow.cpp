#include "uiWindow.h"
#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiWindow::UiWindow(const std::string &id) : _closeButton(UiPool::buttonPool.get())
    {
        setup(id);
        UiWindow::initialize();
    }

    UiWindow::UiWindow(const std::string &id, UiElement *parentElement) : _closeButton(UiPool::buttonPool.get())
    {
        setup(id, parentElement);
        UiWindow::initialize();
    }

    UiWindow::~UiWindow() = default;

    void UiWindow::awake()
    {
    }

    void UiWindow::draw(const float deltaTime)
    {
        BeginScissorMode(static_cast<int>(_scrollView.x), static_cast<int>(_scrollView.y), static_cast<int>(_scrollView.width), static_cast<int>(_scrollView.height));
    }

    void UiWindow::update(const float deltaTime)
    {
        if (!_isAwake)
        {
            awake();
            _isAwake = true;
        }

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
        Editor::getInstance().getEditorModel().getWindowsModel()->addWindowToAllowList(id);
        UiElement::dispose();
        setVerticalResized(false);
        setHorizontalResized(false);
        _isAwake = false;
    }

    void UiWindow::subscribe()
    {
        _closeButton.onClick.subscribe([this](UiButton *_)
        {
            if (!_closeButton.isActive)
            {
                return;
            }

            _closeButton.isActive = false;
            _parent->destroyChild(this);
            getRootElement()->setDirty();
        });
    }

    void UiWindow::unsubscribe()
    {
        _closeButton.onClick.unsubscribeAll();
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
        _closeButton.setup(id + "closeButton", this, "X");
        _closeButton.setSize({15, 15});
        _closeButton.setAnchor(UI_RIGHT_TOP);
        _closeButton.setPivot({1, 1});
        _closeButton.setPosition({-5, 20});
        _closeButton.setRenderOnEndOfFrame();
        _closeButton.setIgnoreScrollLayout();

        Editor::getInstance().getEditorModel().getWindowsModel()->removeWindowFromAllowList(id);
    }

    void UiWindow::calculateRectForScroll(UiElement *element)
    {
        const auto &elementPosition = element->getPosition();
        const auto &elementSize = element->getSize();
        const auto &windowBounds = getBounds();
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
    }
} // BreadEditor

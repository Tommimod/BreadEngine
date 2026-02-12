#include "folderUiElement.h"

#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FolderUiElement::FolderUiElement() : _button(UiPool::labelButtonPool.get()),
                                         _expandButton(UiPool::buttonPool.get())
    {
        _button.setup(id + "button", this, "");
    }

    FolderUiElement::~FolderUiElement() = default;

    FolderUiElement &FolderUiElement::setup(const std::string &id, UiElement *parentElement, Folder *folder)
    {
        _isExpanded = true;
        _engineFolder = folder;
        _button.setText(GuiIconText(ICON_FOLDER, folder->getName().c_str()));
        _button.setTextAlignment(TEXT_ALIGN_LEFT);
        _button.setSizePercentPermanent({1, 1});

        _expandButton.setup(id + "expandButton", this, "");
        updateExpandButtonText();
        _expandButton.setAnchor(UI_LEFT_TOP);
        _expandButton.setPivot({1, 0});
        _expandButton.setPosition({-2, 5});
        _expandButton.setClickOutside(true);
        _expandButton.onClick.subscribe([this](UiButton *)
        {
            _isExpanded = !_isExpanded;
            updateExpandButtonText();
            onExpandStateChanged.invoke(this);
        });

        UiElement::setup(id, parentElement);
        return *this;
    }

    void FolderUiElement::awake()
    {
        const auto size = getSize().y * .5f;
        _expandButton.setSize({size, size});
    }

    void FolderUiElement::draw(const float deltaTime)
    {
        if (Engine::isCollisionPointRec(GetMousePosition(), getBounds()))
        {
            GuiPanel(getBounds(), nullptr);
        }
    }

    void FolderUiElement::update(const float deltaTime)
    {
    }

    void FolderUiElement::dispose()
    {
        _engineFolder = nullptr;
        _isExpanded = false;
        onExpandStateChanged.unsubscribeAll();
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
        UiElement::dispose();
    }

    bool FolderUiElement::tryDeleteSelf()
    {
        UiPool::folderUiElementPool.release(*this);
        return true;
    }

    void FolderUiElement::updateExpandButtonText() const
    {
        _expandButton.setText(_isExpanded ? "-" : "+");
    }
} // BreadEditor

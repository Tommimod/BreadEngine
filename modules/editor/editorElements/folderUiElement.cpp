#include "folderUiElement.h"

#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FolderUiElement::FolderUiElement() : _button(UiPool::labelButtonPool.get())
    {
        _button.setup(id + "button", this, "");
    }

    FolderUiElement::~FolderUiElement() = default;

    FolderUiElement &FolderUiElement::setup(const std::string &id, UiElement *parentElement, Folder *folder)
    {
        _isExpanded = true;
        _engineFolder = folder;
        _button.setText( GuiIconText(ICON_FOLDER, folder->name.c_str()));
        _button.setTextAlignment(TEXT_ALIGN_LEFT);
        _button.setSizePercentPermanent({1, 1});

        UiElement::setup(id, parentElement);
        return *this;
    }

    void FolderUiElement::awake()
    {
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
        onExpanded.unsubscribeAll();
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
        UiElement::dispose();
    }

    bool FolderUiElement::tryDeleteSelf()
    {
        UiPool::folderUiElementPool.release(*this);
        return true;
    }
} // BreadEditor

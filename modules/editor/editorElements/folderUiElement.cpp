#include "folderUiElement.h"
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
        UiElement::setup(id, parentElement);
        return *this;
    }

    FolderUiElement &FolderUiElement::setup(const std::string &id, FolderUiElement *parentElement, Folder *folder)
    {
        _engineFolder = folder;
        _parentFolderElement = parentElement;
        UiElement::setup(id, parentElement);
        return *this;
    }

    void FolderUiElement::awake()
    {
        _button.setText(_engineFolder->name);
        _button.setTextAlignment(TEXT_ALIGN_LEFT);
        _button.setSizePercentPermanent({1, 1});
    }

    void FolderUiElement::draw(const float deltaTime)
    {
    }

    void FolderUiElement::update(const float deltaTime)
    {
    }

    void FolderUiElement::dispose()
    {
        _engineFolder = nullptr;
        _isExpanded = false;
        _parentFolderElement = nullptr;
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

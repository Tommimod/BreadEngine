#include "fileUiElement.h"
#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FileUiElement::FileUiElement() : _assetConfig(Engine::getInstance().getAssetsConfig())
    {
    }

    FileUiElement::~FileUiElement() = default;

    FileUiElement &FileUiElement::setup(const std::string_view &id, UiElement *parentElement, const std::string &fileGuid)
    {
        _fileGuid = fileGuid;
        _file = _assetConfig.getFileByGuid(_fileGuid);
        _button = &UiPool::labelButtonPool.get().setup(TextFormat("%s_button", id), this, "");
        _button->setText(GuiIconText(getIconByFileType(), _file->getShortName().c_str()));
        _button->setTextAlignment(TEXT_ALIGN_LEFT);
        _button->setSizePercentPermanent({1, 1});
        _button->onClick.subscribe([this](const UiLabelButton *)
        {
            onClicked.invoke(this);
        });

        initializeOptionsOwner(this, getOptions());
        UiElement::setup(id, parentElement);
        return *this;
    }

    void FileUiElement::draw(const float deltaTime)
    {
        if (Engine::isCollisionPointRec(GetMousePosition(), getBounds()) || isDragging)
        {
            GuiPanel(getBounds(), nullptr);
        }
    }

    void FileUiElement::update(const float deltaTime)
    {
        updateDraggable(this);
        updateOptionsOwner();
    }

    void FileUiElement::dispose()
    {
        _file = nullptr;
        onClicked.unsubscribeAll();
        _button = nullptr;
        _fileGuid.clear();
        disposeDraggable();
        disposeOptionsOwner();
        UiElement::dispose();
    }

    void FileUiElement::setIsSelected(const bool isSelected)
    {
        setState(isSelected ? STATE_PRESSED : STATE_NORMAL);
    }

    FileUiElement *FileUiElement::copy() const
    {
        const auto copyElement = &UiPool::fileUiElementPool.get().setup(id + "_copy", nullptr, _fileGuid);
        copyElement->setAnchor(_anchor);
        copyElement->setPivot(_pivot);
        copyElement->setBounds(_localPosition, _localSize);
        copyElement->onlyProvideDragEvents = false;
        return copyElement;
    }

    bool FileUiElement::tryDeleteSelf()
    {
        UiPool::fileUiElementPool.release(*this);
        return true;
    }

    void FileUiElement::handleSelectedOption(const int index)
    {
        if (index == 1)
        {
            onRenameRequested.invoke(this);
        }
        else if (index == 2)
        {
            onDeleteRequested.invoke(_fileGuid);
        }
    }

    GuiIconName FileUiElement::getIconByFileType() const
    {
        auto icon = ICON_FILE;
        if (_file->isImage())
        {
            icon = ICON_FILETYPE_IMAGE;
        }
        else if (_file->isAudio())
        {
            icon = ICON_FILETYPE_AUDIO;
        }
        else if (_file->isVideo())
        {
            icon = ICON_FILETYPE_VIDEO;
        }
        else if (_file->isText())
        {
            icon = ICON_FILETYPE_TEXT;
        }
        else if (_file->isConfig())
        {
            icon = ICON_FILETYPE_INFO;
        }
        else if (_file->isNode())
        {
            icon = ICON_CPU;
        }

        return icon;
    }
} // BreadEditor

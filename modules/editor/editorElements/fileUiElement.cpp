#include "fileUiElement.h"
#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FileUiElement::FileUiElement() : _fileSystem(Engine::getInstance().getAssetsRegistry())
    {
    }

    FileUiElement::~FileUiElement() = default;

    FileUiElement &FileUiElement::setup(const std::string &id, UiElement *parentElement, const std::string &fileGuid)
    {
        _fileGuid = fileGuid;
        _file = _fileSystem.getFileByGuid(_fileGuid);
        _button = &UiPool::labelButtonPool.get().setup(id + "button", this, "");
        _button->setText(GuiIconText(ICON_FILE, _file->getShortName().c_str()));
        _button->setTextAlignment(TEXT_ALIGN_LEFT);
        _button->setSizePercentPermanent({1, 1});
        _button->onClick.subscribe([this](const UiLabelButton *)
        {
            onClicked.invoke(this);
        });

        UiElement::setup(id, parentElement);
        return *this;
    }

    void FileUiElement::awake()
    {
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
    }

    void FileUiElement::dispose()
    {
        _file = nullptr;
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
        onClicked.unsubscribeAll();
        _button = nullptr;
        _fileGuid.clear();
        disposeDraggable();
        UiElement::dispose();
    }

    FileUiElement *FileUiElement::copy()
    {
        const auto copyElement = &UiPool::fileUiElementPool.get().setup(id.append("_copy"), nullptr, _fileGuid);
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
} // BreadEditor

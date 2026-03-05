#include "folderUiElement.h"
#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FolderUiElement::FolderUiElement() : _assetConfig(Engine::getInstance().getAssetsConfig())
    {
    }

    FolderUiElement::~FolderUiElement() = default;

    FolderUiElement &FolderUiElement::setup(const std::string &id, UiElement *parentElement, const std::string &folderGuid)
    {
        _isExpanded = true;
        _folderGuid = folderGuid;
        _engineFolder = _assetConfig.getFolderByGuid(_folderGuid);

        _button = &UiPool::labelButtonPool.get().setup(id + "button", this, "");
        _button->setText(GuiIconText(ICON_FOLDER, _engineFolder->getShortName().c_str()));
        _button->setTextAlignment(TEXT_ALIGN_LEFT);
        _button->setSizePercentPermanent({1, 1});

        if (_folderGuid != _assetConfig.getRootFolder()->getGUID())
        {
            _expandButton = &UiPool::buttonPool.get().setup(id + "expandButton", this, "");
            updateExpandButtonText();
            _expandButton->setAnchor(UI_LEFT_TOP);
            _expandButton->setPivot({1, 0});
            _expandButton->setPosition({-2, 5});
            _expandButton->setClickOutside(true);
            _expandButton->setTextSize(static_cast<int>(EditorStyle::FontSize::SmallMedium));
            _expandButton->onClick.subscribe([this](UiButton *)
            {
                _isExpanded = !_isExpanded;
                updateExpandButtonText();
                onExpandStateChanged.invoke(this);
            });
        }

        initializeOptionsOwner(this, {"Rename", "Delete"});
        UiElement::setup(id, parentElement);
        return *this;
    }

    void FolderUiElement::awake()
    {
        if (_expandButton != nullptr)
        {
            const auto size = getSize().y * .5f;
            _expandButton->setSize({size, size});
        }
    }

    void FolderUiElement::draw(const float deltaTime)
    {
        if (Engine::isCollisionPointRec(GetMousePosition(), getBounds()) || isDragging)
        {
            GuiPanel(getBounds(), nullptr);
        }
    }

    void FolderUiElement::update(const float deltaTime)
    {
        updateDraggable(this);
        updateOptionsOwner();
    }

    void FolderUiElement::dispose()
    {
        _engineFolder = nullptr;
        _isExpanded = true;
        onExpandStateChanged.unsubscribeAll();
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
        _button = nullptr;
        _expandButton = nullptr;
        _folderGuid.clear();
        disposeDraggable();
        disposeOptionsOwner();
        UiElement::dispose();
    }

    FolderUiElement *FolderUiElement::copy()
    {
        const auto copyElement = &UiPool::folderUiElementPool.get().setup(id.append("_copy"), nullptr, _folderGuid);
        copyElement->setAnchor(_anchor);
        copyElement->setPivot(_pivot);
        copyElement->setBounds(_localPosition, _localSize);
        copyElement->onlyProvideDragEvents = false;
        return copyElement;
    }

    bool FolderUiElement::tryDeleteSelf()
    {
        UiPool::folderUiElementPool.release(*this);
        return true;
    }

    void FolderUiElement::handleSelectedOption(const int index)
    {
        if (index == 1)
        {
            onRenameRequested.invoke(this);
        }
        else if (index == 2)
        {
            onDeleteRequested.invoke(_folderGuid);
        }
    }

    void FolderUiElement::updateExpandButtonText() const
    {
        _expandButton->setText(_isExpanded ? "-" : "+");
    }
} // BreadEditor

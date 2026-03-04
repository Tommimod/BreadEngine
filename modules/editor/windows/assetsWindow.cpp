#include "assetsWindow.h"
#include <filesystem>
#include <thread>

#include "editor.h"
#include "models/reservedFileNames.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "Assets";
    std::string get_stem(const filesystem::path &p) { return p.stem().string(); }

    AssetsWindow::AssetsWindow(const std::string &id) : UiWindow(id), _fileSystem(Engine::getInstance().getAssetsRegistry())
    {
        setup(id);
        subscribe();
    }

    AssetsWindow::AssetsWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement), _fileSystem(Engine::getInstance().getAssetsRegistry())
    {
        setup(id, parentElement);
        subscribe();
    }

    AssetsWindow::~AssetsWindow() = default;

    void AssetsWindow::awake()
    {
        auto i = 0;
        recalculateUiFolders(_fileSystem.getRootFolder(), i);
    }

    FolderUiElement *AssetsWindow::getFolderUiElementByEngineFolder(const Folder *folder) const
    {
        for (const auto folderInstance: _folderUiElements)
        {
            if (folderInstance != nullptr && folderInstance->getFolderGuid() == folder->getGUID())
            {
                return folderInstance;
            }
        }

        return nullptr;
    }

    FileUiElement *AssetsWindow::getFileUiElementByPath(const File *file) const
    {
        for (const auto fileInstance: _fileUiElements)
        {
            if (fileInstance != nullptr && fileInstance->getFileGuid() == file->getGUID())
            {
                return fileInstance;
            }
        }

        return nullptr;
    }

    FolderUiElement &AssetsWindow::createFolderUiElement(const Folder *folder)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FolderUiElement::elementIdFormat, folder->getPathFromRoot().c_str(), getChildCount());
        auto element = &UiPool::folderUiElementPool.get().setup(id, this, folder->getGUID());

        element->setAnchor(UI_LEFT_TOP);
        element->setSize({0, elementHeight});
        element->setSizePercentPermanent({elementWidthInPercent, -1});
        element->dragContainer = _parent;
        element->onlyProvideDragEvents = true;
        element->onExpandStateChanged.subscribe([this](const FolderUiElement *)
        {
            auto i = 0;
            recalculateUiFolders(_fileSystem.getRootFolder(), i);
        });

        element->onDragStarted.subscribe([this](UiElement *uiElement) { this->onElementDragStarted(uiElement); });
        element->onDragEnded.subscribe([this](UiElement *uiElement) { this->onFolderElementDragEnded(uiElement); });
        _folderUiElements.emplace_back(element);
        return *element;
    }

    FileUiElement &AssetsWindow::createFileUiElement(const File *file)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FileUiElement::elementIdFormat, file->getPathFromRoot().c_str(), getChildCount());
        auto element = &UiPool::fileUiElementPool.get().setup(id, this, file->getGUID());

        element->setAnchor(UI_LEFT_TOP);
        element->setSize({0, elementHeight});
        element->setSizePercentPermanent({elementWidthInPercent, -1});
        element->dragContainer = _parent;
        element->onlyProvideDragEvents = true;
        element->onClicked.subscribe([this](const FileUiElement *fileUiElement)
        {
            onFileSelected(fileUiElement);
        });

        element->onDragStarted.subscribe([this](UiElement *uiElement) { this->onElementDragStarted(uiElement); });
        element->onDragEnded.subscribe([this](UiElement *uiElement) { this->onFileElementDragEnded(uiElement); });
        _fileUiElements.emplace_back(element);
        return *element;
    }

    void AssetsWindow::recalculateUiFolders(Folder *folder, int &nodeOrder, const bool isParentExpanded)
    {
        auto element = getFolderUiElementByEngineFolder(folder);
        if (element == nullptr)
        {
            element = &createFolderUiElement(folder);
        }

        TraceLog(LOG_INFO, folder->getShortName().c_str());
        if (!isParentExpanded)
        {
            element->isActive = false;
            for (auto &file: folder->getFiles())
            {
                recalculateUiFiles(&file, nodeOrder, folder->getDepth(), false);
            }

            for (auto &fld: folder->getFolders())
            {
                recalculateUiFolders(&fld, nodeOrder, false);
            }
            return;
        }

        element->isActive = true;
        element->computeBounds();
        element->update(0);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;
        constexpr float startYPosition = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(folder->getDepth() - 1);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        calculateRectForScroll(element);
        const auto isExpanded = element->getIsExpanded();
        for (auto &file: folder->getFiles())
        {
            recalculateUiFiles(&file, nodeOrder, folder->getDepth(), isExpanded);
        }

        for (auto &fld: folder->getFolders())
        {
            recalculateUiFolders(&fld, nodeOrder, isExpanded);
        }
    }

    void AssetsWindow::recalculateUiFiles(const File *file, int &nodeOrder, const int depth, const bool isParentExpanded)
    {
        auto element = getFileUiElementByPath(file);
        if (element == nullptr)
        {
            element = &createFileUiElement(file);
        }

        if (!isParentExpanded)
        {
            element->isActive = false;
            return;
        }
        TraceLog(LOG_INFO, file->getShortName().c_str());
        element->isActive = true;
        element->computeBounds();
        element->update(0);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;
        constexpr float startYPosition = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(depth);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        calculateRectForScroll(element);
    }

    void AssetsWindow::onFileSelected(const FileUiElement *fileUiElement)
    {
        const auto file = _fileSystem.getFileByGuid(fileUiElement->getFileGuid());
        const auto fileName = file->getShortName().c_str();
        const auto inspectorWindow = &Editor::getInstance().mainWindow.getNodeInspector();
        if (strcmp(fileName, ReservedFileNames::ASSETS_REGISTRY_NAME) == 0)
        {
            inspectorWindow->lookupStruct(&Engine::getInstance().getAssetsRegistry());
        }
    }

    void AssetsWindow::onElementDragStarted(UiElement *uiElement)
    {
        if (const auto folderElement = dynamic_cast<FolderUiElement *>(uiElement); folderElement != nullptr)
        {
            _draggedFolderUiElementCopy = folderElement->copy();
            _draggedFolderUiElementCopy->forceStartDrag(_draggedFolderUiElementCopy);
        }
        else if (const auto fileElement = dynamic_cast<FileUiElement *>(uiElement); fileElement != nullptr)
        {
            _draggedFileUiElementCopy = fileElement->copy();
            _draggedFileUiElementCopy->forceStartDrag(_draggedFileUiElementCopy);
        }
    }

    void AssetsWindow::onFolderElementDragEnded(UiElement *uiElement)
    {
        if (_draggedFolderUiElementCopy == nullptr) return;

        destroyChild(_draggedFolderUiElementCopy);
        _draggedFolderUiElementCopy = nullptr;
        const auto originalElement = dynamic_cast<FolderUiElement *>(uiElement);
        for (const auto folderElement: _folderUiElements)
        {
            const auto folderBounds = folderElement->getBounds();
            if (Engine::isCollisionPointRec(GetMousePosition(), folderBounds))
            {
                if (originalElement == folderElement)
                {
                    return;
                }

                const auto originalFolder = _fileSystem.getFolderByGuid(originalElement->getFolderGuid());
                const auto selectedFolder = _fileSystem.getFolderByGuid(folderElement->getFolderGuid());
                if (originalFolder->tryFindRecursive(selectedFolder))
                {
                    return;
                }

                if (selectedFolder->contains(originalElement->getFolderGuid()))
                {
                    return;
                }

                _fileSystem.moveFolder(originalElement->getFolderGuid(), folderElement->getFolderGuid());
                _fileUiElements.clear();
                _folderUiElements.clear();
                destroyAllChilds();
                std::thread workerThread([this]
                {
                    while (this->getChildCount() != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    }

                    auto i = 0;
                    recalculateUiFolders(_fileSystem.getRootFolder(), i);
                });
                workerThread.detach();
            }
        }
    }

    void AssetsWindow::onFileElementDragEnded(UiElement *uiElement)
    {
        if (_draggedFileUiElementCopy == nullptr) return;

        destroyChild(_draggedFileUiElementCopy);
        _draggedFileUiElementCopy = nullptr;
        const auto originalElement = dynamic_cast<FileUiElement *>(uiElement);
        for (const auto folderElement: _folderUiElements)
        {
            const auto folderBounds = folderElement->getBounds();
            if (Engine::isCollisionPointRec(GetMousePosition(), folderBounds))
            {
                const auto selectedFolder = _fileSystem.getFolderByGuid(folderElement->getFolderGuid());
                if (selectedFolder->contains(originalElement->getFileGuid()))
                {
                    return;
                }

                _fileSystem.moveFile(originalElement->getFileGuid(), folderElement->getFolderGuid());
                _fileUiElements.clear();
                _folderUiElements.clear();
                destroyAllChilds();
                std::thread workerThread([this]
                {
                    while (this->getChildCount() != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    }

                    TraceLog(LOG_INFO, "-----------");
                    auto i = 0;
                    recalculateUiFolders(_fileSystem.getRootFolder(), i);
                });
                workerThread.detach();
            }
        }
    }

    void AssetsWindow::draw(const float deltaTime)
    {
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiScrollPanel(_bounds, _title, _contentView, &_scrollPos, &_scrollView);
        UiWindow::draw(deltaTime);
    }

    void AssetsWindow::update(const float deltaTime)
    {
        UiWindow::update(deltaTime);
    }

    void AssetsWindow::dispose()
    {
        _folderUiElements.clear();
        _fileUiElements.clear();
        _draggedFolderUiElementCopy = nullptr;
        _draggedFileUiElementCopy = nullptr;
        _isUpdateAfterMove = false;
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

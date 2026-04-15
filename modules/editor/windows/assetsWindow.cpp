#include "assetsWindow.h"
#include <filesystem>
#include "editor.h"
#include "models/reservedFileNames.h"
#include "systems/commands/commandsHandler.h"
#include "systems/commands/assetsCommands/deleteAssetCommand.h"
#include "systems/commands/assetsCommands/moveAssetCommand.h"
#include "systems/commands/assetsCommands/renameAssetCommand.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "Assets";
    std::string get_stem(const filesystem::path &p) { return p.stem().string(); }

    AssetsWindow::AssetsWindow(const std::string_view &id) : UiWindow(id), _assetConfig(Engine::getInstance().getAssetsConfig())
    {
        _editorModel = &Editor::getInstance().getEditorModel();

        setup(id);
        subscribe();

        _assetConfig.ConfigUndo.subscribe([this]
        {
            rebuild();
        });
    }

    AssetsWindow::AssetsWindow(const std::string_view &id, UiElement *parentElement) : UiWindow(id, parentElement), _assetConfig(Engine::getInstance().getAssetsConfig())
    {
        _editorModel = &Editor::getInstance().getEditorModel();

        setup(id, parentElement);
        subscribe();

        _assetConfig.ConfigUndo.subscribe([this]
        {
            rebuild();
        });
    }

    AssetsWindow::~AssetsWindow() = default;

    void AssetsWindow::awake()
    {
        UiWindow::awake();

        auto i = 0;
        recalculateUiFolders(_assetConfig.getRootFolder(), i);
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
        auto element = &UiPool::folderUiElementPool.get().setup(id, _content, folder->getGUID());

        element->setAnchor(UI_LEFT_TOP);
        element->setSize({0, elementHeight});
        element->setSizePercentPermanent({elementWidthInPercent, -1});
        element->dragContainer = _parent;
        element->onlyProvideDragEvents = true;
        element->onExpandStateChanged.subscribe([this](const FolderUiElement *)
        {
            auto i = 0;
            recalculateUiFolders(_assetConfig.getRootFolder(), i);
        });

        element->onDragStarted.subscribe([this](UiElement *uiElement) { onElementDragStarted(uiElement); });
        element->onDragEnded.subscribe([this](UiElement *uiElement) { onFolderElementDragEnded(uiElement); });
        element->onDeleteRequested.subscribe([this](const std::string &guid) { deleteAsset(guid); });
        element->onRenameRequested.subscribe([this](const FolderUiElement *uiElement) { renameAsset(uiElement->getFolderGuid()); });
        _folderUiElements.emplace_back(element);
        return *element;
    }

    FileUiElement &AssetsWindow::createFileUiElement(const File *file)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FileUiElement::elementIdFormat, file->getPathFromRoot().c_str(), getChildCount());
        auto element = &UiPool::fileUiElementPool.get().setup(id, _content, file->getGUID());

        element->setAnchor(UI_LEFT_TOP);
        element->setSize({0, elementHeight});
        element->setSizePercentPermanent({elementWidthInPercent, -1});
        element->dragContainer = _parent;
        element->onlyProvideDragEvents = true;
        element->onClicked.subscribe([this](FileUiElement *fileUiElement)
        {
            _editorModel->selectFileUiElement(fileUiElement);
            onFileSelected(fileUiElement);
        });

        element->onDragStarted.subscribe([this](UiElement *uiElement) { onElementDragStarted(uiElement); });
        element->onDragEnded.subscribe([this](UiElement *uiElement) { onFileElementDragEnded(uiElement); });
        element->onDeleteRequested.subscribe([this](const std::string &guid) { deleteAsset(guid); });
        element->onRenameRequested.subscribe([this](const FileUiElement *uiElement) { renameAsset(uiElement->getFileGuid()); });
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
        constexpr float startYPosition = 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(folder->getDepth() - 1);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        _content->calculateRectForScroll(element);
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
        constexpr float startYPosition = 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(depth);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        _content->calculateRectForScroll(element);
    }

    void AssetsWindow::onFileSelected(const FileUiElement *fileUiElement) const
    {
        const auto file = _assetConfig.getFileByGuid(fileUiElement->getFileGuid());
        const auto fileName = file->getShortName().c_str();
        const auto inspectorWindow = &Editor::getInstance().mainWindow.getPropertyInspector();
        if (strcmp(fileName, BreadEngine::ReservedFileNames::ASSETS_REGISTRY_NAME) == 0)
        {
            inspectorWindow->lookupStruct(&Engine::getInstance().getAssetsConfig());
        }
    }

    void AssetsWindow::onElementDragStarted(UiElement *uiElement)
    {
        if (const auto folderElement = dynamic_cast<FolderUiElement *>(uiElement); folderElement != nullptr)
        {
            _draggedFolderUiElementCopy = folderElement->copy();
            _draggedFolderUiElementCopy->forceStartDrag(_draggedFolderUiElementCopy);
            Editor::getInstance().getEditorModel().setDraggableElement(uiElement);
        }
        else if (const auto fileElement = dynamic_cast<FileUiElement *>(uiElement); fileElement != nullptr)
        {
            _draggedFileUiElementCopy = fileElement->copy();
            _draggedFileUiElementCopy->forceStartDrag(_draggedFileUiElementCopy);
            Editor::getInstance().getEditorModel().setDraggableElement(uiElement);
        }
    }

    void AssetsWindow::onFolderElementDragEnded(UiElement *uiElement)
    {
        if (_draggedFolderUiElementCopy == nullptr) return;

        Editor::getInstance().getEditorModel().setDraggableElement(nullptr);
        _content->destroyChild(_draggedFolderUiElementCopy);
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

                const auto originalFolder = _assetConfig.getFolderByGuid(originalElement->getFolderGuid());
                const auto selectedFolder = _assetConfig.getFolderByGuid(folderElement->getFolderGuid());
                if (originalFolder->tryFindRecursive(selectedFolder))
                {
                    return;
                }

                if (selectedFolder->contains(originalElement->getFolderGuid()))
                {
                    return;
                }

                CommandsHandler::execute(std::make_unique<MoveAssetCommand>(originalElement->getFolderGuid(), folderElement->getFolderGuid()));
                rebuild();
            }
        }
    }

    void AssetsWindow::onFileElementDragEnded(UiElement *uiElement)
    {
        if (_draggedFileUiElementCopy == nullptr) return;

        _content->destroyChild(_draggedFileUiElementCopy);
        _draggedFileUiElementCopy = nullptr;
        const auto originalElement = dynamic_cast<FileUiElement *>(uiElement);
        for (const auto folderElement: _folderUiElements)
        {
            const auto folderBounds = folderElement->getBounds();
            if (Engine::isCollisionPointRec(GetMousePosition(), folderBounds))
            {
                const auto selectedFolder = _assetConfig.getFolderByGuid(folderElement->getFolderGuid());
                if (selectedFolder->contains(originalElement->getFileGuid()))
                {
                    return;
                }

                CommandsHandler::execute(std::make_unique<MoveAssetCommand>(originalElement->getFileGuid(), folderElement->getFolderGuid()));
                rebuild();
            }
        }
    }

    void AssetsWindow::renameAsset(const std::string &guid)
    {
        std::string shortName;
        if (const auto file = _assetConfig.getFileByGuid(guid); file != nullptr)
        {
            shortName = file->getShortName();
        }
        else if (const auto folder = _assetConfig.getFolderByGuid(guid); folder != nullptr)
        {
            shortName = folder->getShortName();
        }

        auto &renameWindow = UiPool::renameUiElementPool.get().setup(shortName);
        renameWindow.onApply.subscribe([this, &renameWindow, &guid](std::string &nextText)
        {
            CommandsHandler::execute(std::make_unique<RenameAssetCommand>(guid, nextText));
            renameWindow.getParentElement()->destroyChild(&renameWindow);
            rebuild();
        });
    }

    void AssetsWindow::deleteAsset(const std::string &guid)
    {
        CommandsHandler::execute(std::make_unique<DeleteAssetCommand>(guid));
        rebuild();
    }

    void AssetsWindow::rebuild()
    {
        _fileUiElements.clear();
        _folderUiElements.clear();
        _content->destroyAllChilds();
        auto i = 0;
        recalculateUiFolders(_assetConfig.getRootFolder(), i);
    }

    void AssetsWindow::initializePanel()
    {
    }

    void AssetsWindow::cleanupPanel()
    {
    }

    void AssetsWindow::update(const float deltaTime)
    {
        const auto selectedFile = _editorModel->getSelectedFileUiElement();
        for (const auto fileUiElement: _fileUiElements)
        {
            fileUiElement->setIsSelected(selectedFile != nullptr && selectedFile->id == fileUiElement->id);
        }

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

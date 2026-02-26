#include "assetsWindow.h"
#include <filesystem>
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
            if (folderInstance && &folderInstance->getFolder() == folder)
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
            if (fileInstance && fileInstance->getFile() == file)
            {
                return fileInstance;
            }
        }

        return nullptr;
    }

    FolderUiElement &AssetsWindow::CreateFolderUiElement(Folder *folder)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FolderUiElement::elementIdFormat, folder->getName().c_str(), getChildCount());
        auto &element = UiPool::folderUiElementPool.get().setup(id, this, folder);

        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});
        element.dragContainer = _parent;
        element.onlyProvideDragEvents = true;
        element.onExpandStateChanged.subscribe([this](const FolderUiElement *)
        {
            auto i = 0;
            recalculateUiFolders(_fileSystem.getRootFolder(), i);
        });

        _folderUiElements.emplace_back(&element);
        return element;
    }

    FileUiElement &AssetsWindow::CreateFileUiElement(File *file)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FileUiElement::elementIdFormat, file->getPathFromRoot().c_str(), getChildCount());
        auto &element = UiPool::fileUiElementPool.get().setup(id, this, file);

        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});
        element.dragContainer = _parent;
        element.onlyProvideDragEvents = true;

        _fileUiElements.emplace_back(&element);
        element.onClicked.subscribe([this](const FileUiElement *fileUiElement)
        {
            onFileSelected(fileUiElement);
        });
        return element;
    }

    void AssetsWindow::recalculateUiFolders(Folder *folder, int &nodeOrder, const bool isParentExpanded)
    {
        auto isExpanded = true;
        if (nodeOrder > 0)
        {
            auto element = getFolderUiElementByEngineFolder(folder);
            if (!element)
            {
                element = &CreateFolderUiElement(folder);
            }
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
            isExpanded = element->getIsExpanded();
        }

        for (auto &file: folder->getFiles())
        {
            recalculateUiFiles(&file, nodeOrder, folder->getDepth(), isExpanded);
        }

        for (auto &fld: folder->getFolders())
        {
            recalculateUiFolders(&fld, nodeOrder, isExpanded);
        }
    }

    void AssetsWindow::recalculateUiFiles(File *file, int &nodeOrder, const int depth, const bool isParentExpanded)
    {
        auto element = getFileUiElementByPath(file);
        if (!element)
        {
            element = &CreateFileUiElement(file);
        }

        if (!isParentExpanded)
        {
            element->isActive = false;
            return;
        }

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
        const auto fileName = fileUiElement->getFile()->getPathFromRoot().c_str();
        const auto inspectorWindow = &Editor::getInstance().mainWindow.getNodeInspector();
        if (strcmp(fileName, BreadEngine::ReservedFileNames::ASSETS_REGISTRY_NAME) == 0)
        {
            inspectorWindow->lookupStruct(&Engine::getInstance().getAssetsRegistry());
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

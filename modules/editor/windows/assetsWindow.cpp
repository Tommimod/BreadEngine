#include "assetsWindow.h"
#include <filesystem>
#include "editor.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string AssetsWindow::Id = "Assets";
    std::string get_stem(const filesystem::path &p) { return p.stem().string(); }

    AssetsWindow::AssetsWindow(const std::string &id) : UiWindow(id), _fileSystem(Engine::getInstance().getFileSystem())
    {
        setup(id);
        subscribe();
    }

    AssetsWindow::AssetsWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement), _fileSystem(Engine::getInstance().getFileSystem())
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

    FileUiElement * AssetsWindow::getFileUiElementByPath(const std::string &fileName) const
    {
        for (const auto fileInstance: _fileUiElements)
        {
            if (fileInstance && fileInstance->getFileName() == fileName.c_str())
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

        const auto id = TextFormat(FolderUiElement::elementIdFormat, folder->name.c_str(), getChildCount());
        auto &element = UiPool::folderUiElementPool.get().setup(id, this, folder);

        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});
        element.dragContainer = _parent;
        element.onlyProvideDragEvents = true;

        _folderUiElements.emplace_back(&element);
        return element;
    }

    FileUiElement & AssetsWindow::CreateFileUiElement(const std::string &fileName)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(FileUiElement::elementIdFormat, fileName.c_str(), getChildCount());
        auto &element = UiPool::fileUiElementPool.get().setup(id, this, fileName);

        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});
        element.dragContainer = _parent;
        element.onlyProvideDragEvents = true;

        _fileUiElements.emplace_back(&element);
        return element;
    }

    void AssetsWindow::recalculateUiFolders(Folder *folder, int &nodeOrder, const bool isParentExpanded)
    {
        auto element = getFolderUiElementByEngineFolder(folder);
        if (!element)
        {
            element = &CreateFolderUiElement(folder);
        }

        element->isActive = true;
        element->computeBounds();
        element->update(0);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;
        constexpr float startYPosition = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(folder->depth + 1);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        calculateRectForScroll(element);
        if (!isParentExpanded)
        {
            return;
        }

        for (const auto &file: folder->files)
        {
            if (file.empty()) continue;
            TraceLog(LOG_INFO, file.c_str());
            recalculateUiFiles(file, nodeOrder, folder->depth + 1);
        }

        TraceLog(LOG_INFO, "Folders:");
        for (auto &fld: folder->folders)
        {
            if (fld.isEmpty()) continue;
            recalculateUiFolders(&fld, nodeOrder, element->getIsExpanded());
        }
    }

    void AssetsWindow::recalculateUiFiles(const std::string &fileName, int &nodeOrder, const int depth)
    {
        auto element = getFileUiElementByPath(fileName);
        if (!element)
        {
            element = &CreateFileUiElement(fileName);
        }

        element->isActive = true;
        element->computeBounds();
        element->update(0);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;
        constexpr float startYPosition = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(depth + 1);

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        calculateRectForScroll(element);
    }

    void AssetsWindow::draw(const float deltaTime)
    {
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

#include "assetsWindow.h"
#include <filesystem>
#include "editor.h"

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
        createFolderElement(_fileSystem.getRootFolder());
    }

    void AssetsWindow::createFolderElement(const Folder &folder)
    {
        TraceLog(LOG_INFO, "----- Next folder: %s ------", folder.name.c_str());
        TraceLog(LOG_INFO, "Files:");
        for (const auto &file: folder.files)
        {
            if (file.empty()) continue;
            TraceLog(LOG_INFO, file.c_str());
        }

        TraceLog(LOG_INFO, "Folders:");
        for (const auto &fld: folder.folders)
        {
            if (fld.isEmpty()) continue;
            createFolderElement(fld);
        }
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

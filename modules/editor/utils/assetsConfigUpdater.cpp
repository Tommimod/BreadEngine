#include "assetsConfigUpdater.h"

#include "engine.h"

namespace BreadEditor {
    BreadEngine::SubscriptionHandle AssetsConfigUpdater::_subscription{};

    void AssetsConfigUpdater::subscribe()
    {
        _subscription = FilesWatcher::onFileChanged.subscribe(onUpdate);
    }

    void AssetsConfigUpdater::unsubscribe()
    {
        FilesWatcher::onFileChanged.unsubscribe(_subscription);
    }

    void AssetsConfigUpdater::onUpdate(const FilesWatcher::CallbackData &data)
    {
        auto &assetsConfig = BreadEngine::Engine::getInstance().getAssetsConfig();
        const auto projectPath = BreadEngine::Engine::getProjectPath();
        switch (data.action)
        {
            case FilesWatcher::ACTION_CREATE: assetsConfig.onFileCreated(projectPath + data.path);
                break;
            case FilesWatcher::ACTION_DELETE: assetsConfig.onFileDeleted(projectPath + data.path);
                break;
            case FilesWatcher::ACTION_MODIFY:
                //TODO check if .h or .cpp and re-build game module
                break;
            case FilesWatcher::ACTION_MOVE: assetsConfig.onFileMoved(projectPath + data.oldPath, projectPath + data.path);
                break;
        }
    }
} // BreadEditor

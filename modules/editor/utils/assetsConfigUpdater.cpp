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
        auto path = data.rootdir + data.path;
        switch (data.action)
        {
            case FilesWatcher::ACTION_CREATE: assetsConfig.onEntityCreated(path);
                break;
            case FilesWatcher::ACTION_DELETE: assetsConfig.onEntityDeleted(path);
                break;
            case FilesWatcher::ACTION_MODIFY:
                //TODO check if .h or .cpp and re-build game module
                break;
            case FilesWatcher::ACTION_MOVE: auto oldPath = data.rootdir + data.oldPath;
                assetsConfig.onEntityMoved(oldPath, path);
                break;
        }
    }
} // BreadEditor

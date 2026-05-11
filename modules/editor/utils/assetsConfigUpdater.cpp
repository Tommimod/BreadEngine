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
        switch (data.action)
        {
            case FilesWatcher::ACTION_CREATE: assetsConfig.onEntityCreated(data.rootdir + data.path);
                break;
            case FilesWatcher::ACTION_DELETE: assetsConfig.onEntityDeleted(data.rootdir + data.path);
                break;
            case FilesWatcher::ACTION_MODIFY:
                //TODO check if .h or .cpp and re-build game module
                break;
            case FilesWatcher::ACTION_MOVE: assetsConfig.onEntityMoved(data.rootdir + data.oldPath, data.rootdir + data.path);
                break;
        }
    }
} // BreadEditor

#include "deleteAssetCommand.h"
#include "engine.h"

namespace BreadEditor {
    DeleteAssetCommand::DeleteAssetCommand(const std::string &assetGuid)
        : _assetsConfig(BreadEngine::Engine::getInstance().getAssetsConfig())
    {
        _assetGuid = assetGuid;
    }

    void DeleteAssetCommand::execute()
    {
        if (const auto file = _assetsConfig.getFileByGuid(_assetGuid); file != nullptr)
        {
            _assetsConfig.deleteFile(_assetGuid);
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_assetGuid); folder != nullptr)
        {
            _assetsConfig.deleteFolder(_assetGuid);
        }
    }

    void DeleteAssetCommand::undo()
    {
    }
} // BreadEditor

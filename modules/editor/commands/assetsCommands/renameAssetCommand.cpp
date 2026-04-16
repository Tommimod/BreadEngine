#include "renameAssetCommand.h"
#include "engine.h"

namespace BreadEditor {
    RenameAssetCommand::RenameAssetCommand(const std::string &assetGuid, const std::string &nextName)
        : _assetsConfig(BreadEngine::Engine::getInstance().getAssetsConfig())
    {
        _assetGuid = assetGuid;
        _nextName = nextName;
        if (const auto file = _assetsConfig.getFileByGuid(_assetGuid); file != nullptr)
        {
            _oldName = file->getShortName();
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_assetGuid); folder != nullptr)
        {
            _oldName = folder->getShortName();
        }
    }

    void RenameAssetCommand::execute()
    {
        if (const auto file = _assetsConfig.getFileByGuid(_assetGuid); file != nullptr)
        {
            _assetsConfig.renameFile(_assetGuid, _nextName);
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_assetGuid); folder != nullptr)
        {
            _assetsConfig.renameFolder(_assetGuid, _nextName);
        }
    }

    void RenameAssetCommand::undo()
    {
        if (const auto file = _assetsConfig.getFileByGuid(_assetGuid); file != nullptr)
        {
            _assetsConfig.renameFile(_assetGuid, _oldName);
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_assetGuid); folder != nullptr)
        {
            _assetsConfig.renameFolder(_assetGuid, _oldName);
        }

        _assetsConfig.ConfigUndo.invoke();
    }
} // BreadEditor

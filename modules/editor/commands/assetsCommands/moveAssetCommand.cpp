#include "moveAssetCommand.h"
#include "engine.h"

namespace BreadEditor {
    MoveAssetCommand::MoveAssetCommand(const std::string &currentAssetGuid, const std::string &nextFolderGuid)
        : _assetsConfig(BreadEngine::Engine::getInstance().getAssetsConfig())
    {
        _currentAssetGuid = currentAssetGuid;
        _nextFolderGuid = nextFolderGuid;
        if (const auto file = _assetsConfig.getFileByGuid(_currentAssetGuid); file != nullptr)
        {
            const auto oldFolderPath = std::string(GetDirectoryPath(file->getFullPath().c_str()));
            const auto oldFolder = _assetsConfig.getFolderByPath(oldFolderPath);
            _oldFolderGuid = oldFolder->getGUID();
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_currentAssetGuid); folder != nullptr)
        {
            const auto currentFolder = _assetsConfig.getFolderByGuid(_currentAssetGuid);
            const auto oldFolderPath = std::string(GetPrevDirectoryPath(currentFolder->getFullPath().c_str()));
            const auto oldFolder = _assetsConfig.getFolderByPath(oldFolderPath);
            _oldFolderGuid = oldFolder->getGUID();
        }
    }

    void MoveAssetCommand::execute()
    {
        if (const auto file = _assetsConfig.getFileByGuid(_currentAssetGuid); file != nullptr)
        {
            _assetsConfig.moveFile(_currentAssetGuid, _nextFolderGuid);
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_currentAssetGuid); folder != nullptr)
        {
            _assetsConfig.moveFolder(_currentAssetGuid, _nextFolderGuid);
        }
    }

    void MoveAssetCommand::undo()
    {
        if (const auto file = _assetsConfig.getFileByGuid(_currentAssetGuid); file != nullptr)
        {
            _assetsConfig.moveFile(_currentAssetGuid, _oldFolderGuid);
        }
        else if (const auto folder = _assetsConfig.getFolderByGuid(_currentAssetGuid); folder != nullptr)
        {
            _assetsConfig.moveFolder(_currentAssetGuid, _oldFolderGuid);
        }

        _assetsConfig.ConfigUndo.invoke();
    }
}

#pragma once
#include <string>
#include "configs/assetsConfig.h"
#include "commands/command.h"

namespace BreadEditor {
    struct MoveAssetCommand : Command
    {
        MoveAssetCommand(const std::string &currentAssetGuid, const std::string &nextFolderGuid);

        ~MoveAssetCommand() override = default;

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _currentAssetGuid;
        std::string _nextFolderGuid;
        std::string _oldFolderGuid;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
}

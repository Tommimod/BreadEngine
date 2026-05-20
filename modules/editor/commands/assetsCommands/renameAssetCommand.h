#pragma once
#include <string>

#include "../../../engine/configs/assets/assetsConfig.h"
#include "commands/command.h"

namespace BreadEditor {
    struct RenameAssetCommand : Command
    {
        RenameAssetCommand(const std::string &assetGuid, const std::string &nextName);

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _assetGuid;
        std::string _oldName;
        std::string _nextName;
    };
} // BreadEditor

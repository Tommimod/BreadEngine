#pragma once
#include <string>

#include "configs/assetsConfig.h"
#include "systems/commands/command.h"

namespace BreadEditor {
    struct RenameAssetCommand : Command
    {
        RenameAssetCommand(const std::string &assetGuid, const std::string &nextName);

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _assetGuid;
        std::string _oldName;
        std::string _nextName;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor

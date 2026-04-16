#pragma once
#include <string>
#include "configs/assetsConfig.h"
#include "commands/command.h"

namespace BreadEditor {
    struct DeleteAssetCommand : Command
    {
        explicit DeleteAssetCommand(const std::string &assetGuid);

        ~DeleteAssetCommand() override = default;

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _assetGuid;

        bool withUndo() override { return false; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor

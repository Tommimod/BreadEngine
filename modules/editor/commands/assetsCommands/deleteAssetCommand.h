#pragma once
#include <string>
#include "configs/assetsConfig.h"
#include "commands/command.h"

namespace BreadEditor {
    struct DeleteAssetCommand : Command
    {
        explicit DeleteAssetCommand(const std::string &assetGuid);

        ~DeleteAssetCommand() override = default;

        bool withUndo() override { return false; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _assetGuid;
    };
} // BreadEditor

#pragma once
#include <string>
#include "../../../engine/configs/assets/assetsConfig.h"
#include "commands/command.h"

namespace BreadEditor {
    /// Creates a new material asset in a project folder.
    struct CreateMaterialCommand : Command
    {
        explicit CreateMaterialCommand(const std::string &folderGuid);

        ~CreateMaterialCommand() override = default;

        bool withUndo() override { return false; }

        void execute() override;

        void undo() override;

    private:
        BreadEngine::AssetsConfig &_assetsConfig;
        std::string _folderGuid;

        /// The first free `New Material<n>.mat` in @p folderPath. Creating two materials in a row
        /// must not have the second one land on the first.
        [[nodiscard]] static std::string nextFreePath(const std::string &folderPath);
    };
} // BreadEditor

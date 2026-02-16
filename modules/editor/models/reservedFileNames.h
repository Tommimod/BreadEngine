#pragma once

namespace BreadEditor {
    class ReservedFileNames
    {
    public:
        constexpr static auto MARKER_CONFIG = ".cnf";
        constexpr static auto EDITOR_PREFS_NAME = "editor_prefs.cnf";
        constexpr static auto EDITOR_IN_PROJECT_SETTINGS_NAME = "editor_settings.cnf";
    };
} // BreadEditor
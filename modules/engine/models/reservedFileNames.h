#pragma once

namespace BreadEngine {
    class ReservedFileNames
    {
    public:
        constexpr static auto MARKER_NODE = ".nd";
        constexpr static auto PROJECT_SETTINGS_NAME = "project_settings.cnf";
        constexpr static auto ASSETS_REGISTRY_NAME = "assets_registry.cnf";
        constexpr static auto EDITOR_IN_PROJECT_SETTINGS_NAME = "editor_settings.cnf";
        constexpr static auto GLOBAL_LIGHT_SETTINGS_NAME = "global_light_settings.cnf";
    };
} // BreadEngine

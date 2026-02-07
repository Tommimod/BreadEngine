#pragma once
#include <memory>

#include "../editorPrefsConfig.h"

namespace BreadEditor {
    class ConfigsProvider
    {
    public:
        ConfigsProvider() = default;

        ~ConfigsProvider() = default;

        [[nodiscard]] EditorPrefsConfig *getEditorPrefsConfig() const
        {
            return _editorPrefsConfig.get();
        }

        void setEditorPrefsConfig(std::unique_ptr<EditorPrefsConfig> editorPrefsConfig)
        {
            _editorPrefsConfig = std::move(editorPrefsConfig);
        }

    private:
        std::unique_ptr<EditorPrefsConfig> _editorPrefsConfig;
    };
} // BreadEditor

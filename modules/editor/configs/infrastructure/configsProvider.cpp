#include "configsProvider.h"

namespace BreadEditor {
    ConfigsProvider::~ConfigsProvider()
    {
        _editorPrefsConfig->serialize();
    }
} // BreadEditor
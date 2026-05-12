#include "asset.h"

#include "engine.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(Asset)

    void Asset::loadToMemory()
    {
    }

    File *Asset::getFile() const
    {
        return Engine::getInstance().getAssetsConfig().getFileByGuid(_fileGuid).get();
    }
} // BreadEngine

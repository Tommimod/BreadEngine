#pragma once
#include "inspectorObject.h"
#include "../configs/assets/materialAsset.h"
#include "../rendering/renderHandles.h"

namespace BreadEngine {
    /**
     * Which material a renderer draws one of its slots with. It holds no surface of its own:
     * material data lives in the project as a .mat asset shared by every slot that links it,
     * which is what makes one edit there reach all of them.
     */
    struct MaterialLink : InspectorStruct
    {
        [[nodiscard]] bool isLinked() const { return _materialAsset != nullptr; }

        /**
         * The linked material's renderer material, or the renderer's default surface while the
         * slot links nothing - an unlinked slot draws untextured rather than not at all.
         */
        [[nodiscard]] MaterialHandle getHandle() const;

    private:
        MaterialAsset *_materialAsset = nullptr;
        /**
         * Asks for a material of this slot's own rather than the shared one. Instanced materials
         * do not exist yet, so the box is inert: the slot still draws with what the link names.
         */
        bool _isInstanced = false;

        INSPECTOR_BEGIN(MaterialLink)
            INSPECT_FIELD(_materialAsset);
            INSPECT_FIELD(_isInstanced);
        INSPECTOR_END()
    };
} // BreadEngine

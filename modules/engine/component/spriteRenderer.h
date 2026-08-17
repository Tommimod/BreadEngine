#pragma once
#include "../configs/assets/textureAsset.h"
#include "core/component.h"
#include "data/materialLink.h"
#include "rendering/renderHandles.h"

namespace BreadEngine {
    struct SpriteRenderer : Component
    {
        SpriteRenderer() = default;

        explicit SpriteRenderer(Node *owner);

        ~SpriteRenderer() override = default;

        void onDestroy() override;

        void loadSprite(Vector3 forward);

        void unload();

        [[nodiscard]] bool isLoaded() const;

        /// Whether the quad no longer matches the linked texture. The inspector can clear or
        /// replace that link at any time and raises no flag doing so.
        [[nodiscard]] bool isQuadStale();

        /**
         * The material this sprite draws with: the linked one, or the one built from its own
         * texture while it links none. The link cannot supply the texture itself - a material
         * asset is shared, and its texture set is fixed once it exists.
         */
        [[nodiscard]] MaterialHandle getMaterialHandle() const;

        void setTextureAsset(TextureAsset *textureAsset, Vector3 forward);

    private:
        friend class SpriteRendererSystem;
        MeshHandle _mesh{};
        MaterialLink _material;
        TextureAsset *_textureAsset = nullptr;
        /// The material built from _textureAsset, drawn while the slot links none of its own.
        MaterialHandle _textureMaterial{};
        /**
         * The asset the texture reference was taken from, and the texture the quad was sized
         * against. The asset resolving to a different one is how a texture assigned through the
         * inspector reaches the mesh; the asset itself is kept because the inspector rewrites
         * _textureAsset behind this component's back, and the reference has to go back to where
         * it came from.
         */
        TextureAsset *_acquiredTexture = nullptr;
        TextureHandle _quadTexture{};
        bool _isLoaded = false;

        INSPECTOR_BEGIN(SpriteRenderer)
            INSPECT_FIELD(_textureAsset)
            INSPECT_FIELD(_material)
        INSPECTOR_END()
    };
} // BreadEngine

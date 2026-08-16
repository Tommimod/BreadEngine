#pragma once
#include "../configs/assets/textureAsset.h"
#include "core/component.h"
#include "data/material.h"
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

        void setTextureAsset(TextureAsset *textureAsset, Vector3 forward);

    private:
        friend class SpriteRendererSystem;
        MeshHandle _mesh{};
        Material _material;
        TextureAsset *_textureAsset = nullptr;
        /// The texture the quad was sized against. The asset resolving to a different one is
        /// how a texture assigned through the inspector reaches the mesh. Only ever compared,
        /// never resolved, so it holds no reference of its own.
        TextureHandle _quadTexture{};
        bool _isLoaded = false;

        INSPECTOR_BEGIN(SpriteRenderer)
            INSPECT_FIELD(_textureAsset)
            INSPECT_FIELD(_material)
        INSPECTOR_END()
    };
} // BreadEngine

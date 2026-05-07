#pragma once
#include "r3d_mesh.h"
#include "configs/textureAsset.h"
#include "core/component.h"
#include "data/material.h"

namespace BreadEngine {
    struct SpriteRenderer : Component
    {
        SpriteRenderer() = default;

        explicit SpriteRenderer(Node *owner);

        ~SpriteRenderer() override = default;

        void loadSprite(Vector3 forward);

        void unload();

        [[nodiscard]] bool isLoaded() const;

        void setTextureAsset(TextureAsset *textureAsset, Vector3 forward);

    private:
        friend class SpriteRendererSystem;
        R3D_Mesh _nativeMeshRenderer = {};
        Material _material;
        TextureAsset *_textureAsset = nullptr;
        bool _isLoaded = false;

        INSPECTOR_BEGIN(SpriteRenderer)
            INSPECT_FIELD(_textureAsset)
            INSPECT_FIELD(_material)
        INSPECTOR_END()
    };
} // BreadEngine

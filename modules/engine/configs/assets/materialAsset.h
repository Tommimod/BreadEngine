#pragma once
#include <array>

#include "asset.h"
#include "textureAsset.h"
#include "rendering/renderHandles.h"

namespace BreadEngine {
    /**
     * A surface authored in the project, shared by every renderer slot that links it: one GPU
     * material, so an edit here reaches all of them at once. The .mat file is the material -
     * the fields below are what it holds, which is what makes a change to one of them show up
     * in a diff as that one material.
     */
    struct MaterialAsset : Asset
    {
        MaterialAsset() : Asset()
        {
        }

        explicit MaterialAsset(const std::string &fileGuid) : Asset(fileGuid)
        {
        }

        ~MaterialAsset() override;

        [[nodiscard]] bool isStoredInOwnFile() const override { return true; }

        /**
         * Reads the material's fields from its file, once. The GPU material is not built here -
         * that is left to the first slot that asks for it.
         *
         * Must run outside a deserialization phase: the texture links resolve through the generic
         * deserializer, which inside a phase defers them, and a deferred link only ever reaches a
         * component. Loading with the assets registry, before any scene, is what satisfies that.
         */
        void loadToMemory() override;

        void saveToOwnFile() override;

        /**
         * The renderer material for this surface, built on first use and rebuilt whenever the
         * linked textures no longer resolve to what it was built from. That comparison is the
         * only invalidation there is: the inspector writes the texture fields directly, so
         * nothing can be hung off a setter.
         */
        [[nodiscard]] MaterialHandle getHandle();

    private:
        /// Texture slots, in the order MaterialDesc declares them.
        static constexpr size_t TEXTURE_SLOTS = 4;

        using TextureLinks = std::array<TextureAsset *, TEXTURE_SLOTS>;

        /**
         * One texture as the material was built: the asset the reference was taken from, and
         * the handle it gave. The link fields below can be reassigned behind the material's
         * back, so the reference has to be given back to the asset it was taken from rather
         * than to whatever the link names by then.
         */
        struct AcquiredTexture
        {
            TextureAsset *asset = nullptr;
            TextureHandle handle{};
        };

        std::string _shaderPath;
        MaterialHandle _handle{};
        std::array<AcquiredTexture, TEXTURE_SLOTS> _acquired{};
        /// Whether getHandle() has already tried to build a material. Set even when the
        /// renderer refuses, so a failure costs one attempt rather than one every frame.
        bool _isResolved = false;
        /// Whether the file has been read. Re-reading it would discard edits the inspector has
        /// made and not saved yet.
        bool _isLoaded = false;
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_omrTexture = nullptr;
        TextureAsset *_emissionTexture = nullptr;

        /// Releases the renderer material and the texture references it was built from, leaving
        /// the next getHandle() to build it again.
        void unload();

        [[nodiscard]] TextureLinks textureLinks() const;

        /// Whether @p links still resolve to the textures _handle was built from.
        [[nodiscard]] bool isBuiltFrom(const TextureLinks &links);

        INSPECTOR_BEGIN(MaterialAsset)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_omrTexture);
            INSPECT_FIELD(_emissionTexture);
        INSPECTOR_END()
    };
} // BreadEngine

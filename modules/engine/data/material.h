#pragma once
#include <array>

#include "inspectorObject.h"
#include "../configs/assets/textureAsset.h"
#include "../rendering/renderTypes.h"

namespace BreadEngine {
    struct Material : InspectorStruct
    {
        Material() = default;

        /**
         * A copy carries the asset links and none of the GPU state - two owners of one binding
         * would free it twice, and give back one texture reference each. The copy resolves a
         * material of its own on first use, so copying is always correct; at worst it rebuilds.
         */
        Material(const Material &other);

        Material &operator=(const Material &other);

        ~Material() override;

        void setShaderPath(const std::string &path) { _shaderPath = path; }

        /// Releases the renderer material and the texture references it was built from. The
        /// destructor does the same, so this is only needed to force a rebuild.
        void unload();

        /**
         * The renderer material for this surface, built on first use and rebuilt whenever the
         * linked assets no longer resolve to what it was built from. That comparison is the
         * only invalidation there is: the inspector writes the texture fields directly, so
         * nothing can be hung off the setters.
         */
        [[nodiscard]] MaterialHandle getHandle();

        void setAlbedoTexture(TextureAsset *texture) { _albedoTexture = texture; }
        void setNormalTexture(TextureAsset *texture) { _normalTexture = texture; }
        void setOmrTexture(TextureAsset *texture) { _omrTexture = texture; }
        void setEmissionTexture(TextureAsset *texture) { _emissionTexture = texture; }

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
        TextureAsset *_albedoTexture = nullptr;
        TextureAsset *_normalTexture = nullptr;
        TextureAsset *_omrTexture = nullptr;
        TextureAsset *_emissionTexture = nullptr;

        [[nodiscard]] TextureLinks textureLinks() const;

        /// Whether @p links still resolve to the textures _handle was built from.
        [[nodiscard]] bool isBuiltFrom(const TextureLinks &links);

        void copyLinksFrom(const Material &other);

        INSPECTOR_BEGIN(Material)
            INSPECT_FIELD(_shaderPath);
            INSPECT_FIELD(_albedoTexture);
            INSPECT_FIELD(_normalTexture);
            INSPECT_FIELD(_omrTexture);
            INSPECT_FIELD(_emissionTexture);
        INSPECTOR_END()
    };
} // BreadEngine

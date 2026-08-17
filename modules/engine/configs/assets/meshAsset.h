#pragma once
#include <vector>

#include "asset.h"
#include "data/material.h"
#include "rendering/renderTypes.h"
#include "rendering/geometry/modelImporter.h"

namespace BreadEngine {
    struct MeshAsset : Asset
    {
        MeshAsset() : Asset()
        {
        }

        explicit MeshAsset(const std::string &fileGuid) : Asset(fileGuid)
        {
        }

        ~MeshAsset() override = default;

        [[nodiscard]] std::vector<Material> const &getMaterials() const { return _materials; }

        void loadToMemory() override;

        /**
         * Takes a reference for a holder about to store the parts, importing and uploading the
         * geometry unless another holder already has. Each part names a slot in getMaterials().
         * Empty when the model cannot be imported, in which case no reference was taken and
         * release() must not be called.
         */
        [[nodiscard]] const std::vector<MeshPart> &acquire();

        /// Gives back one acquire()'s reference, freeing the geometry once none are left.
        void release();

    private:
        std::vector<Material> _materials;
        std::vector<MeshPart> _parts;
        /// How many holders acquired this model's geometry. The material list is loaded eagerly
        /// with the registry, but the geometry is not: this count is all that keeps it resident.
        unsigned int _references = 0;
        bool _isLoaded = false;

        /// Imports the source file and uploads what it holds. The CPU-side geometry is dropped
        /// once uploaded, so a model no holder wants costs nothing but its material list.
        void importParts();

        /**
         * Points each material at the textures its counterpart in the model file names. A slot
         * the file leaves unnamed falls back to the file-name convention, which is all the
         * import had before it read materials at all.
         */
        void wireTextures(const std::vector<ModelMaterial> &declared);

        /// The project's texture asset for @p pathFromModel as the model file spells it -
        /// relative to the model, with either separator - or nullptr if the project has no
        /// such file.
        [[nodiscard]] static TextureAsset *findTexture(const std::string &modelDirectory, const std::string &pathFromModel);

        /**
         * The texture asset named after the model file, as `textures/<model><suffix>` in any of
         * the image formats the convention allows. This is how a model whose own material names
         * no texture - or names one that is not in the project - still gets one.
         */
        [[nodiscard]] static TextureAsset *findConventionalTexture(const std::string &modelDirectory, const std::string &modelName, const char *suffix);

        INSPECTOR_BEGIN(MeshAsset)
            INSPECT_FIELD(_materials);
        INSPECTOR_END()
    };
} // BreadEngine

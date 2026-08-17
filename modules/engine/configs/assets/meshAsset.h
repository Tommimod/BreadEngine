#pragma once
#include <vector>

#include "asset.h"
#include "rendering/renderTypes.h"

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

        /// Nothing to read ahead of use: a model no holder wants costs nothing, and the first
        /// acquire() is what imports it.
        void loadToMemory() override
        {
        }

        /**
         * Takes a reference for a holder about to store the parts, importing and uploading the
         * geometry unless another holder already has. Each part names one of the model's material
         * slots. Empty when the model cannot be imported, in which case no reference was taken
         * and release() must not be called.
         */
        [[nodiscard]] const std::vector<MeshPart> &acquire();

        /// Gives back one acquire()'s reference, freeing the geometry once none are left.
        void release();

    private:
        std::vector<MeshPart> _parts;
        /// How many holders acquired this model's geometry - the only thing keeping it resident.
        unsigned int _references = 0;

        /// Imports the source file and uploads what it holds. The CPU-side geometry is dropped
        /// once uploaded.
        void importParts();
    };
} // BreadEngine

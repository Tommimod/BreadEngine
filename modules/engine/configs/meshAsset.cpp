#include "meshAsset.h"

#include "engine.h"
#include "r3d_model.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MeshAsset)

    MeshAsset::MeshAsset(File *file) : Asset(file)
    {
        const auto mesh = R3D_LoadModel(getAssetPath().c_str());
        for (int i = 0; i < mesh.materialCount; i++)
        {
            _materials.push_back(Material());
        }

        if (mesh.materialCount == 0) return;

        const char *folder = "textures";
        const char *albedo_name = "_albedo";
        const char *normal_name = "_normal";
        const char *omr_name = "_omr";
        const char *jpg_format = ".jpg";
        const char *jpeg_format = ".jpeg";
        const char *png_format = ".png";

        const auto formats = {jpg_format, jpeg_format, png_format};
        auto &material = _materials[0];
        const auto fileName = GetFileNameWithoutExt(file->getFullPath().c_str());
        const auto directory = GetDirectoryPath(file->getFullPath().c_str());
        const std::string albedoPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, albedo_name);
        const std::string normalPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, normal_name);
        const std::string omrPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, omr_name);
        auto &assetsConfig = Engine::getInstance().getAssetsConfig();
        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", albedoPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = static_cast<TextureAsset *>(assetsConfig.getAsset(textureFile));
            material.setAlbedoTexture(textureAsset);
        }

        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", normalPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = static_cast<TextureAsset *>(assetsConfig.getAsset(textureFile));
            material.setNormalTexture(textureAsset);
        }

        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", omrPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = static_cast<TextureAsset *>(assetsConfig.getAsset(textureFile));
            material.setOmrTexture(textureAsset);
        }
    }
} // BreadEngine

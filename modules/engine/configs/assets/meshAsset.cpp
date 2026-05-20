#include "meshAsset.h"

#include "engine.h"
#include "r3d_model.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MeshAsset)

    void MeshAsset::loadToMemory()
    {
        if (_isLoaded) return;
        if (!_materials.empty())
        {
            _isLoaded = true;
            return;
        }

        _isLoaded = true;
        auto path= getAssetPath().c_str();
        const auto mesh = R3D_LoadModel(path);
        for (int i = 0; i < mesh.materialCount; i++)
        {
            _materials.emplace_back();
        }

        if (mesh.materialCount == 0) return;

        const char *folder = "textures";
        const char *albedo_name = "_albedo";
        const char *normal_name = "_normal";
        const char *omr_name = "_omr";
        const char *emission_name = "_emission";
        const char *jpg_format = ".jpg";
        const char *jpeg_format = ".jpeg";
        const char *png_format = ".png";

        const auto formats = {jpg_format, jpeg_format, png_format};
        auto &material = _materials[0];
        auto file = getFile();
        const auto fileName = GetFileNameWithoutExt(file->getFullPath().c_str());
        const auto directory = GetDirectoryPath(file->getFullPath().c_str());
        const std::string albedoPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, albedo_name);
        const std::string normalPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, normal_name);
        const std::string omrPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, omr_name);
        const std::string emissionPath = TextFormat("%s\\%s\\%s%s", directory, folder, fileName, emission_name);
        auto &assetsConfig = Engine::getInstance().getAssetsConfig();
        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", albedoPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = dynamic_cast<TextureAsset *>(assetsConfig.getAsset(textureFile).get());
            material.setAlbedoTexture(textureAsset);
        }

        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", normalPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = dynamic_cast<TextureAsset *>(assetsConfig.getAsset(textureFile).get());
            material.setNormalTexture(textureAsset);
        }

        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", omrPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = dynamic_cast<TextureAsset *>(assetsConfig.getAsset(textureFile).get());
            material.setOmrTexture(textureAsset);
        }

        for (auto &format: formats)
        {
            const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s%s", emissionPath.c_str(), format));
            if (!textureFile) continue;
            const auto textureAsset = dynamic_cast<TextureAsset *>(assetsConfig.getAsset(textureFile).get());
            material.setEmissionTexture(textureAsset);
        }

        R3D_UnloadModel(mesh, true);
    }
} // BreadEngine

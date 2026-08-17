#include "meshAsset.h"

#include <algorithm>

#include "engine.h"
#include "rendering/renderer.h"

namespace BreadEngine {
    DEFINE_STATIC_PROPS(MeshAsset)

    void MeshAsset::loadToMemory()
    {
        if (_isLoaded) return;

        _isLoaded = true;
        if (!_materials.empty()) return;

        const ModelData model = importModel(getAssetPath());
        _materials.resize(model.materials.size());
        wireTextures(model.materials);
    }

    const std::vector<MeshPart> &MeshAsset::acquire()
    {
        if (_parts.empty()) importParts();
        if (!_parts.empty()) ++_references;

        return _parts;
    }

    void MeshAsset::release()
    {
        if (_references == 0) return;
        if (--_references > 0) return;

        auto &renderer = Renderer::get();
        for (const auto &[mesh, materialSlot]: _parts) renderer.destroyMesh(mesh);

        _parts.clear();
    }

    void MeshAsset::importParts()
    {
        const ModelData model = importModel(getAssetPath());
        auto &renderer = Renderer::get();

        _parts.reserve(model.parts.size());
        for (const auto &[geometry, materialSlot]: model.parts)
        {
            const auto mesh = renderer.createMesh(geometry);
            if (mesh.isValid()) _parts.push_back(MeshPart{.mesh = mesh, .materialSlot = materialSlot});
        }
    }

    void MeshAsset::wireTextures(const std::vector<ModelMaterial> &declared)
    {
        struct MaterialSlot
        {
            std::string ModelMaterial::*declared;
            const char *conventionalSuffix;
            void (Material::*apply)(TextureAsset *);
        };

        constexpr MaterialSlot SLOTS[]{
            {&ModelMaterial::albedo, "_albedo", &Material::setAlbedoTexture},
            {&ModelMaterial::normal, "_normal", &Material::setNormalTexture},
            {&ModelMaterial::orm, "_omr", &Material::setOmrTexture},
            {&ModelMaterial::emission, "_emission", &Material::setEmissionTexture}
        };

        const auto file = getFile();
        const std::string directory = GetDirectoryPath(file->getFullPath().c_str());
        const std::string modelName = GetFileNameWithoutExt(file->getFullPath().c_str());

        for (size_t slot = 0; slot < _materials.size() && slot < declared.size(); ++slot)
        {
            for (const auto &[declaredPath, suffix, apply]: SLOTS)
            {
                auto *texture = findTexture(directory, declared[slot].*declaredPath);
                if (texture == nullptr && slot == 0) texture = findConventionalTexture(directory, modelName, suffix);
                if (texture != nullptr) (_materials[slot].*apply)(texture);
            }
        }
    }

    TextureAsset *MeshAsset::findTexture(const std::string &modelDirectory, const std::string &pathFromModel)
    {
        if (pathFromModel.empty()) return nullptr;

        std::string relativePath = pathFromModel;
        std::ranges::replace(relativePath, '/', '\\');

        auto &assetsConfig = Engine::getInstance().getAssetsConfig();
        const auto textureFile = assetsConfig.getFileByPath(TextFormat("%s\\%s", modelDirectory.c_str(), relativePath.c_str()));
        if (!textureFile) return nullptr;

        return dynamic_cast<TextureAsset *>(assetsConfig.getAsset(textureFile).get());
    }

    TextureAsset *MeshAsset::findConventionalTexture(const std::string &modelDirectory, const std::string &modelName, const char *suffix)
    {
        constexpr const char *FORMATS[]{".jpg", ".jpeg", ".png"};
        for (const auto *format: FORMATS)
        {
            if (auto *texture = findTexture(modelDirectory, TextFormat("textures\\%s%s%s", modelName.c_str(), suffix, format)))
            {
                return texture;
            }
        }

        return nullptr;
    }
} // BreadEngine

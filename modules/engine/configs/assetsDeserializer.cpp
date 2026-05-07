#include "assetsDeserializer.h"

#include "engine.h"
#include "inspectorObject.h"
#include "nameof.h"
#include <yaml-cpp/node/node.h>

#include "meshAsset.h"

namespace BreadEngine {
    static std::string getTypeName(const char *typeName)
    {
        int status;
        auto *mangled = abi::__cxa_demangle(typeName, nullptr, nullptr, &status);
        std::string result = status == 0 ? mangled : typeName;
        std::free(mangled);

        if (const size_t lastColon = result.rfind("::"); lastColon != std::string::npos)
        {
            return result.substr(lastColon + 2);
        }
        return result;
    }

    void AssetsDeserializer::deserialize(YAML::Node &node)
    {
        constexpr static std::string typeKey = "type";
        auto &rhs = Engine::getInstance().getAssetsConfig();
        const auto guidToAssetName = NAMEOF(rhs._guidToAsset).c_str();

        auto assetNode = node[guidToAssetName].as<YAML::Node>();
        InspectorStruct::beginDeserializationPhase();
        std::vector<YAML::detail::iterator_value> meshValues;
        for (auto iteratorValues: assetNode)
        {
            auto dataType = iteratorValues.second[typeKey].as<std::string>();
            if (TextIsEqual(dataType.c_str(), getTypeName(typeid(MeshAsset).name()).c_str()))
            {
                meshValues.emplace_back(iteratorValues);
            }
            else if (TextIsEqual(dataType.c_str(), getTypeName(typeid(TextureAsset).name()).c_str()))
            {
                auto key = iteratorValues.first.as<std::string>();
                auto data = iteratorValues.second.as<YAML::Node>();
                rhs._guidToAsset[key] = new TextureAsset(key);
                auto asset = dynamic_cast<TextureAsset *>(rhs._guidToAsset[key]);
                asset->deserialize(data);
            }
        }
        InspectorStruct::resolveAllDeferredAssetLinks();
        InspectorStruct::endDeserializationPhase();

        for (const auto &iteratorValues: meshValues)
        {
            auto key = iteratorValues.first.as<std::string>();
            auto data = iteratorValues.second.as<YAML::Node>();
            rhs._guidToAsset[key] = new MeshAsset(key);
            auto asset = dynamic_cast<MeshAsset *>(rhs._guidToAsset[key]);
            asset->deserialize(data);
        }
    }
} // BreadEngine

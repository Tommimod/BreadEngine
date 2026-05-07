#pragma once
#include "inspectorObject.h"
#include "models/reservedFileNames.h"
#include <yaml-cpp/node/node.h>
#include <string>

namespace BreadEngine {
    struct File : InspectorStruct
    {
        File() = default;

        File(const std::string &fullName, const std::string &pathFromRoot, const std::string &extension, const std::string &guid)
        {
            this->_fullPath = fullName;
            this->_pathFromRoot = pathFromRoot;
            this->_extension = extension;
            this->_guid = guid;
            this->_shortName = GetFileName(_fullPath.c_str());
        }

        ~File() override = default;

        bool operator==(const File &other) const
        {
            return _guid == other._guid && _fullPath == other._fullPath;
        }

        bool operator!=(const File &other) const
        {
            return !(*this == other);
        }

        bool operator==(const File *other) const
        {
            return _guid == other->_guid && _fullPath == other->_fullPath;
        }

        bool operator!=(const File *other) const
        {
            return this != other;
        }

        [[nodiscard]] const std::string &getGUID() const { return _guid; }
        [[nodiscard]] const std::string &getFullPath() const { return _fullPath; }
        [[nodiscard]] const std::string &getPathFromRoot() const { return _pathFromRoot; }
        [[nodiscard]] const std::string &getExtension() const { return _extension; }
        [[nodiscard]] const std::string &getShortName() const { return _shortName; }

        //File types
        [[nodiscard]] bool isImage() const
        {
            constexpr auto jpg = ".jpg";
            constexpr auto jpeg = ".jpeg";
            constexpr auto png = ".png";
            constexpr auto ico = ".ico";
            return _extension == jpg || _extension == jpeg || _extension == png || _extension == ico;
        }

        [[nodiscard]] bool is3DModel() const
        {
            constexpr auto obj = ".obj";
            constexpr auto fbx = ".fbx";
            constexpr auto gltf = ".gltf";
            constexpr auto glb = ".glb";
            return _extension == obj || _extension == fbx || _extension == gltf || _extension == glb;
        }

        [[nodiscard]] bool isAudio() const
        {
            constexpr auto mp3 = ".mp3";
            constexpr auto wav = ".wav";
            constexpr auto ogg = ".ogg";
            constexpr auto m4a = ".m4a";
            return _extension == mp3 || _extension == wav || _extension == ogg || _extension == m4a;
        }

        [[nodiscard]] bool isVideo() const
        {
            constexpr auto mp4 = ".mp4";
            constexpr auto mov = ".mov";
            constexpr auto avi = ".avi";
            return _extension == mp4 || _extension == mov || _extension == avi;
        }

        [[nodiscard]] bool isText() const
        {
            constexpr auto txt = ".txt";
            constexpr auto json = ".json";
            constexpr auto xml = ".xml";
            constexpr auto yaml = ".yaml";
            return _extension == txt || _extension == json || _extension == xml || _extension == yaml;
        }

        [[nodiscard]] bool isConfig() const
        {
            constexpr auto cnf = ".cnf";
            return _extension == cnf;
        }

        [[nodiscard]] bool isNode() const
        {
            constexpr auto node = ReservedFileNames::MARKER_NODE;
            return _extension == node;
        }

        [[nodiscard]] bool IsUnexpectedType() const
        {
            return !isImage() && !is3DModel() && !isAudio() && !isConfig() && !isNode() && !isText() && !isVideo();
        }

    private:
        friend struct YAML::convert<File>;
        friend struct AssetsConfig;
        std::string _guid;
        std::string _fullPath;
        std::string _pathFromRoot;
        std::string _extension;
        std::string _shortName;

        INSPECTOR_BEGIN(File)
            INSPECT_FIELD(_guid);
            INSPECT_FIELD(_fullPath);
            INSPECT_FIELD(_shortName);
            INSPECT_FIELD(_extension);
        INSPECTOR_END()
    };
} // BreadEngine

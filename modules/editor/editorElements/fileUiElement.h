#pragma once
#include "action.h"
#include "../../engine/configs/assetsConfig.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiLabelButton.h"

namespace BreadEditor {
    class FileUiElement : public UiElement, public IUiDraggable
    {
    public:
        static constexpr auto elementIdFormat = "FileInsT_%s_%d";
        BreadEngine::Action<FileUiElement *> onClicked;

        FileUiElement();

        ~FileUiElement() override;

        [[nodiscard]] FileUiElement &setup(const std::string &id, UiElement *parentElement, const std::string &fileGuid);

        [[nodiscard]] const std::string &getFileGuid() const { return _fileGuid; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        FileUiElement *copy();

    protected:
        bool tryDeleteSelf() override;

    private:
        AssetsConfig &_assetConfig;
        File *_file = nullptr;
        UiLabelButton *_button = nullptr;
        std::string _fileGuid;
    };
} // BreadEditor

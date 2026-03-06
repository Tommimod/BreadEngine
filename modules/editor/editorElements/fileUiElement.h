#pragma once
#include "action.h"
#include "../../engine/configs/assetsConfig.h"
#include "uitoolkit/IUiDraggable.h"
#include "uitoolkit/IUiOptionsOwner.h"
#include "uitoolkit/uiElement.h"
#include "uitoolkit/uiLabelButton.h"

namespace BreadEditor {
    class FileUiElement : public UiElement, public IUiDraggable, public IUiOptionsOwner
    {
    public:
        static constexpr auto elementIdFormat = "FileInsT_%s_%d";
        Action<FileUiElement *> onClicked;
        Action<FileUiElement *> onRenameRequested;
        Action<std::string &> onDeleteRequested;

        FileUiElement();

        ~FileUiElement() override;

        [[nodiscard]] FileUiElement &setup(const std::string &id, UiElement *parentElement, const std::string &fileGuid);

        [[nodiscard]] const std::string &getFileGuid() const { return _fileGuid; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setIsSelected(bool isSelected);

        FileUiElement *copy();

        [[nodiscard]] virtual std::vector<std::string> &getOptions()
        {
            return _options;
        }

    protected:
        bool tryDeleteSelf() override;

        void handleSelectedOption(int index) override;

    private:
        std::vector<std::string> _options = {"Rename", "Delete"};
        AssetsConfig &_assetConfig;
        File *_file = nullptr;
        UiLabelButton *_button = nullptr;
        std::string _fileGuid;
    };
} // BreadEditor

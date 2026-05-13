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

        [[nodiscard]] FileUiElement &setup(const std::string_view &id, UiElement *parentElement, const std::string &fileGuid);

        [[nodiscard]] const std::string &getFileGuid() const { return _fileGuid; }

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setIsSelected(bool isSelected);

        FileUiElement *copy() const;

        void highlight();

    protected:
        bool tryDeleteSelf() override;

        std::vector<std::string> getOptions() override;

        void handleSelectedOption(int index) override;

    private:
        const std::vector<std::string> _options = {"Rename", "Delete"};
        AssetsConfig &_assetConfig;
        std::shared_ptr<File> _file = nullptr;
        UiLabelButton *_button = nullptr;
        std::string _fileGuid;

        float _highlightTimer = 0.0f;
        bool _isHighlighting = false;
        float _originalX = 0.0f;
        float _currentShakeOffset = 0.0f;
        int _shakeDirection = 1;

        GuiIconName getIconByFileType() const;
    };
} // BreadEditor

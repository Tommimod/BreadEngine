#pragma once
#include "action.h"
#include "systems/fileSystem.h"
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

        [[nodiscard]] FileUiElement &setup(const std::string &id, UiElement *parentElement, File *file);

        [[nodiscard]] File *getFile() const { return _file; }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        File *_file = nullptr;
        UiLabelButton &_button;
    };
} // BreadEditor

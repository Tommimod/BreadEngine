#pragma once
#include "action.h"
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

        [[nodiscard]] FileUiElement &setup(const std::string &id, UiElement *parentElement, const std::string &fileName);

        [[nodiscard]] const char *getFileName() const { return _fileName.c_str(); }

        void awake() override;

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

    protected:
        bool tryDeleteSelf() override;

    private:
        std::string _fileName;
        UiLabelButton &_button;
    };
} // BreadEditor

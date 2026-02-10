#include "fileUiElement.h"

#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    FileUiElement::FileUiElement() : _button(UiPool::labelButtonPool.get())
    {
        _button.setup(id + "button", this, "");
    }

    FileUiElement::~FileUiElement() = default;

    FileUiElement &FileUiElement::setup(const std::string &id, UiElement *parentElement, const std::string &fileName)
    {
        _fileName = fileName;

        _button.setText(GuiIconText(ICON_FILE, getFileName()));
        _button.setTextAlignment(TEXT_ALIGN_LEFT);
        _button.setSizePercentPermanent({1, 1});

        UiElement::setup(id, parentElement);
        return *this;
    }

    void FileUiElement::awake()
    {
    }

    void FileUiElement::draw(const float deltaTime)
    {
        if (Engine::isCollisionPointRec(GetMousePosition(), getBounds()))
        {
            GuiPanel(getBounds(), nullptr);
        }
    }

    void FileUiElement::update(const float deltaTime)
    {
    }

    void FileUiElement::dispose()
    {
        onDragEnded.unsubscribeAll();
        onDragStarted.unsubscribeAll();
        UiElement::dispose();
    }

    bool FileUiElement::tryDeleteSelf()
    {
        UiPool::fileUiElementPool.release(*this);
        return true;
    }
} // BreadEditor

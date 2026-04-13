#include "renameUiElement.h"
#include "editor.h"
#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    RenameUiElement &RenameUiElement::setup(const std::string &fromName)
    {
        _text = fromName;
        const auto root = &Editor::getInstance().mainWindow;
        UiElement::setup(_title, root);

        setAnchor(UI_CENTER_CENTER);
        setPivot({.5f, .5f});
        setSize({200, 80});
        enableOverlayLayer();
        computeBounds();
        return *this;
    }

    void RenameUiElement::awake()
    {
        _textBox = &UiPool::textBoxPool.get().setup(id + "_text", this, _text, static_cast<int>(EditorStyle::FontSize::MediumLarge), true);
        _textBox->setAnchor(UI_CENTER_TOP);
        _textBox->setPivot({.5f, 0});
        _textBox->setPosition({0, 5});
        _textBox->setSize({0, 20});
        _textBox->setSizePercentPermanent({.9f, -1});
        _textBox->onValueChanged.subscribe([this](const char *text)
        {
            _text = text;
            if (_text.empty())
            {
                _applyButton->setState(STATE_DISABLED);
            }
            else
            {
                _applyButton->setState(STATE_NORMAL);
            }
        });

        _applyButton = &UiPool::buttonPool.get().setup(id + "_applyButton", this, "Apply");
        _applyButton->setAnchor(UI_CENTER_TOP);
        _applyButton->setPivot({.5f, 0});
        _applyButton->setPosition({0, 30});
        _applyButton->setSize({0, 18});
        _applyButton->setSizePercentPermanent({.3f, -1});
        _applyButton->onClick.subscribe([this](UiButton *)
        {
            onApply.invoke(_text);
        });

        _textBox->setState(STATE_PRESSED);
    }

    void RenameUiElement::draw(const float deltaTime)
    {
        if (GuiWindowBox(getBounds(), _title))
        {
            getParentElement()->destroyChild(this);
        }
    }

    void RenameUiElement::update(const float deltaTime)
    {
        if (!Engine::isCollisionPointRec(GetMousePosition(), getBounds())
            && (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
        {
            getParentElement()->destroyChild(this);
        }
    }

    void RenameUiElement::dispose()
    {
        _applyButton = nullptr;
        _text.clear();
        onApply.unsubscribeAll();
        UiElement::dispose();
    }

    bool RenameUiElement::tryDeleteSelf()
    {
        UiPool::renameUiElementPool.release(*this);
        return true;
    }
} // BreadEditor

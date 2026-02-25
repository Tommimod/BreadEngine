#include "uiTextBox.h"

#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    constexpr int BUFFER_SIZE = 127;
    UiTextBox::UiTextBox() = default;

    UiTextBox &UiTextBox::setup(const std::string &id, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _textSize = defaultTextSize;
        _textBuffer.resize(BUFFER_SIZE + 1);
        std::strncpy(_textBuffer.data(), _internalText.c_str(), BUFFER_SIZE);
        _textBuffer[BUFFER_SIZE] = '\0';
        _text = _textBuffer.data();
        _editMode = defaultEditMode;
        UiElement::setup(id);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, const std::string &defaultText, const int defaultTextSize, const bool defaultEditMode)
    {
        _internalText = defaultText;
        _textSize = defaultTextSize;
        _textBuffer.resize(BUFFER_SIZE + 1);
        std::strncpy(_textBuffer.data(), _internalText.c_str(), BUFFER_SIZE);
        _textBuffer[BUFFER_SIZE] = '\0';
        _text = _textBuffer.data();
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox &UiTextBox::setup(const std::string &id, UiElement *parentElement, std::function<std::string()> getFunc, const int defaultTextSize, const bool defaultEditMode)
    {
        _getFunc = std::move(getFunc);
        _internalText = _getFunc();
        _textSize = defaultTextSize;
        _textBuffer.resize(BUFFER_SIZE + 1);
        std::strncpy(_textBuffer.data(), _internalText.c_str(), BUFFER_SIZE);
        _textBuffer[BUFFER_SIZE] = '\0';
        _text = _textBuffer.data();
        _editMode = defaultEditMode;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiTextBox::~UiTextBox() = default;

    void UiTextBox::dispose()
    {
        onValueChanged.unsubscribeAll();
        onValueChangedWithSender.unsubscribeAll();
        _text = nullptr;
        _getFunc = nullptr;
        _internalText.clear();
        _textSize = 0;
        _editMode = false;
        _textBuffer.clear();
        UiElement::dispose();
    }

    void UiTextBox::draw(const float deltaTime)
    {
        GuiSetStyle(DEFAULT, TEXT_SIZE, _textSize);
        GuiTextBox(_bounds, _text, BUFFER_SIZE, _editMode);
    }

    void UiTextBox::update(const float deltaTime)
    {
        if (_state == STATE_DISABLED)
        {
            return;
        }

        if (Engine::isCollisionPointRec(GetMousePosition(), _bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = true;
        }
        else if (_editMode && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            _editMode = false;
            _internalText = std::string(_text);
            onValueChanged.invoke(_text);
            onValueChangedWithSender.invoke(_text, this);
        }

        if (_editMode && IsKeyDown(KEY_ENTER))
        {
            _editMode = false;
            _internalText = std::string(_text);
            onValueChanged.invoke(_text);
            onValueChangedWithSender.invoke(_text, this);
        }

        if (!_editMode && _getFunc != nullptr)
        {
            _internalText = _getFunc();
            std::strncpy(_textBuffer.data(), _internalText.c_str(), BUFFER_SIZE);
            _textBuffer[BUFFER_SIZE] = '\0';
        }
    }

    void UiTextBox::setText(const std::string &newText)
    {
        _internalText = newText;
        _textBuffer.resize(BUFFER_SIZE + 1);
        std::strncpy(_textBuffer.data(), _internalText.c_str(), BUFFER_SIZE);
        _textBuffer[BUFFER_SIZE] = '\0';
        _text = _textBuffer.data();
    }

    bool UiTextBox::tryDeleteSelf()
    {
        UiPool::textBoxPool.release(*this);
        return true;
    }
} // BreadEditor

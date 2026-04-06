#include "messageUiElement.h"

#include "engine.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    MessageUiElement::MessageUiElement() = default;

    MessageUiElement::~MessageUiElement() = default;

    constexpr std::string empty = "";

    MessageUiElement &MessageUiElement::setup(const std::string_view &id, UiElement *parentElement, const Logger::LogEntity &logEntity)
    {
        _logEntity = logEntity;
        UiElement::setup(id, parentElement);

        _button = &UiPool::labelButtonPool.get().setup(this->id + "_button", this, empty);
        _button->setSizePercentPermanent({1, 1});
        _button->onClick.subscribe([this](UiLabelButton *) { onClick.invoke(_logEntity); });

        GuiIconName icon = ICON_NONE;
        if (logEntity.level == Logger::Info) icon = ICON_INFO;
        else if (logEntity.level == Logger::Warning) icon = ICON_WARNING;
        else if (logEntity.level == Logger::Error) icon = ICON_DEMON;
        _text = GuiIconText(icon, GetFirstNLines(_logEntity.message, 2).c_str());

        auto &text = UiPool::labelPool.get().setup(this->id + "_text", this, _text);
        text.setSizePercentPermanent({1, 1});
        text.setTextAlignment(TEXT_ALIGN_LEFT);
        text.setVerticalAlignment(TEXT_ALIGN_TOP);
        return *this;
    }

    void MessageUiElement::draw(const float deltaTime)
    {
        GuiPanel(getBounds(), nullptr);
    }

    void MessageUiElement::dispose()
    {
        onClick.unsubscribeAll();
        _text.clear();
        _logEntity = {};
        _button = nullptr;
        UiElement::dispose();
    }

    bool MessageUiElement::tryDeleteSelf()
    {
        UiPool::messageUiElementPool.release(*this);
        return true;
    }

    std::string MessageUiElement::GetFirstNLines(std::string_view text, const size_t maxLines)
    {
        size_t pos = 0;
        size_t lineCount = 0;

        while (lineCount < maxLines)
        {
            const size_t next = text.find('\n', pos);
            if (next == std::string_view::npos) return std::string(text);

            ++lineCount;
            pos = next + 1;
        }

        return std::string(text.substr(0, pos - 1));
    }
} // BreadEditor

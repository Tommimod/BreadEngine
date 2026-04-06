#pragma once
#include "logger.h"
#include "uitoolkit/uiLabelButton.h"

namespace BreadEditor {
    class MessageUiElement : public UiElement
    {
    public:
        Action<Logger::LogEntity &> onClick;

        MessageUiElement();

        ~MessageUiElement() override;

        [[nodiscard]] MessageUiElement &setup(const std::string_view &id, UiElement *parentElement, const Logger::LogEntity &logEntity);

        void draw(float deltaTime) override;

        void dispose() override;

    protected:
        Logger::LogEntity _logEntity;
        std::string _text;

        bool tryDeleteSelf() override;

    private:
        static std::string GetFirstNLines(std::string_view text, size_t maxLines = 2);
    };
} // BreadEditor

#include "editorStyle.h"

#include "raygui.h"
#include "raylib.h"
#include "tracy/Tracy.hpp"

namespace BreadEditor {
    Font EditorStyle::_fontLarge = {};
    Font EditorStyle::_fontLargeSmall = {};
    Font EditorStyle::_fontMedium = {};
    Font EditorStyle::_fontMediumLarge = {};
    Font EditorStyle::_fontSmall = {};
    Font EditorStyle::_fontSmallMedium = {};

    int EditorStyle::_lastFontSize = 0;
    GuiState EditorStyle::_lastGuiState = STATE_NORMAL;
    int EditorStyle::_lastLabelTextAlignment = -1;
    int EditorStyle::_lastDrowDownBoxTextAlignment = -1;
    int EditorStyle::_lastButtonTextAlignment = -1;
    int EditorStyle::_lastGlobalTextVerticalAlignment = -1;
    int EditorStyle::_lastGlobalTextWrapMode = -1;

    void EditorStyle::setFontSize(FontSize size)
    {
        ZoneScoped;
        if (_lastFontSize == static_cast<int>(size)) return;
        _lastFontSize = static_cast<int>(size);
        switch (size)
        {
            case FontSize::Small: GuiSetFont(_fontSmall);
                break;
            case FontSize::SmallMedium: GuiSetFont(_fontSmallMedium);
                break;
            case FontSize::Medium: GuiSetFont(_fontMedium);
                break;
            case FontSize::MediumLarge: GuiSetFont(_fontMediumLarge);
                break;
            case FontSize::LargeSmall: GuiSetFont(_fontLargeSmall);
                break;
            case FontSize::Large: GuiSetFont(_fontLarge);
                break;
            default: throw std::runtime_error("Invalid font size");
        }

        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize);
    }

    void EditorStyle::setFontSize(int size)
    {
        if (_lastFontSize == size) return;
        _lastFontSize = size;
        setFontSize(static_cast<FontSize>(size));
    }

    void EditorStyle::setGlobalState(const GuiState state)
    {
        ZoneScoped;
        if (_lastGuiState == state) return;
        _lastGuiState = state;
        GuiSetState(state);
    }

    void EditorStyle::setLabelTextAlignment(const GuiTextAlignment alignment)
    {
        ZoneScoped;
        if (_lastLabelTextAlignment == alignment) return;
        _lastLabelTextAlignment = alignment;
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, alignment);
    }

    void EditorStyle::setDrowDownBoxTextAlignment(const GuiTextAlignment alignment)
    {
        ZoneScoped;
        if (_lastDrowDownBoxTextAlignment == alignment) return;
        _lastDrowDownBoxTextAlignment = alignment;
        GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, alignment);
    }

    void EditorStyle::setButtonTextAlignment(const GuiTextAlignment alignment)
    {
        ZoneScoped;
        if (_lastButtonTextAlignment == alignment) return;
        _lastButtonTextAlignment = alignment;
        GuiSetStyle(BUTTON, TEXT_ALIGNMENT, alignment);
    }

    void EditorStyle::setGlobalTextVerticalAlignment(const GuiTextAlignmentVertical alignment)
    {
        ZoneScoped;
        if (_lastGlobalTextVerticalAlignment == alignment) return;
        _lastGlobalTextVerticalAlignment = alignment;
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, alignment);
    }

    //Super heavy function
    void EditorStyle::setGlobalTextWrapMode(const GuiTextWrapMode mode)
    {
        ZoneScoped;
        if (_lastGlobalTextWrapMode == mode) return;
        _lastGlobalTextWrapMode = mode;
        GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, mode);
    }

    void EditorStyle::loadFont()
    {
        ZoneScoped;
        const auto path = TextFormat("%s/%s", GetApplicationDirectory(), R"(assets\editor\fonts\Roboto.ttf)");
        _fontSmall = LoadFontEx(path, static_cast<int>(FontSize::Small), nullptr, 0);
        _fontSmallMedium = LoadFontEx(path, static_cast<int>(FontSize::SmallMedium), nullptr, 0);
        _fontMedium = LoadFontEx(path, static_cast<int>(FontSize::Medium), nullptr, 0);
        _fontMediumLarge = LoadFontEx(path, static_cast<int>(FontSize::MediumLarge), nullptr, 0);
        _fontLargeSmall = LoadFontEx(path, static_cast<int>(FontSize::LargeSmall), nullptr, 0);
        _fontLarge = LoadFontEx(path, static_cast<int>(FontSize::Large), nullptr, 0);

        _lastFontSize = static_cast<int>(FontSize::Medium);
        GuiSetFont(_fontMedium);
    }
} // BreadEditor

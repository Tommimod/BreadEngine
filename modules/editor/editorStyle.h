#pragma once
#include "raygui.h"

namespace BreadEditor {
    class EditorStyle
    {
    public:
        enum class FontSize
        {
            Small = 10,
            SmallMedium = 12,
            Medium = 14,
            MediumLarge = 16,
            LargeSmall = 18,
            Large = 20
        };

        static void setFontSize(FontSize size);

        static void setFontSize(int size);

        static void setGlobalState(GuiState state);

        static void setLabelTextAlignment(GuiTextAlignment alignment);

        static void setDrowDownBoxTextAlignment(GuiTextAlignment alignment);

        static void setButtonTextAlignment(GuiTextAlignment alignment);

        static void setGlobalTextVerticalAlignment(GuiTextAlignmentVertical alignment);

        static void setGlobalTextWrapMode(GuiTextWrapMode mode);

    private:
        friend class Editor;
        static Font _fontSmall;
        static Font _fontSmallMedium;
        static Font _fontMedium;
        static Font _fontMediumLarge;
        static Font _fontLargeSmall;
        static Font _fontLarge;

        static int _lastFontSize;
        static GuiState _lastGuiState;
        static int _lastLabelTextAlignment;
        static int _lastDrowDownBoxTextAlignment;
        static int _lastButtonTextAlignment;
        static int _lastGlobalTextVerticalAlignment;
        static int _lastGlobalTextWrapMode;

        static void loadFont();
    };
} // BreadEditor

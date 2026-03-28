/* Start Header ************************************************************************/
/*!
\file FontManager.h
\author Joash Ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date March 28, 2026
\brief This file declares the FontManager singleton for text rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include <map>
#include <string>
#include <vector>
#include <functional>

// Text alignment modes
enum class TextAlignment {
    Left,
    Center,
    Right
};

// Predefined color themes
enum class FontTheme {
    White,          // (1, 1, 1, 1)
    Black,          // (0, 0, 0, 1)
    Red,            // (1, 0, 0, 1)
    Green,          // (0, 1, 0, 1)
    Blue,           // (0, 0, 1, 1)
    Yellow,         // (1, 1, 0, 1)
    Cyan,           // (0, 1, 1, 1)
    Magenta,        // (1, 0, 1, 1)
    Orange,         // (1, 0.5, 0, 1)
    Purple,         // (0.5, 0, 1, 1)
    Gold,           // (1, 0.84, 0, 1)
    DebugGreen,     // (0.2, 1.0, 0.3, 1) - for debug overlay
    DebugCyan,      // (0.2, 1.0, 0.35, 1) - for debug input
    HUDGold,        // (1.0, 0.95, 0.2, 1) - for HUD elements
    Dim,            // (0.6, 0.6, 0.6, 1) - for dimmed text
    Transparent     // (1, 1, 1, 0) - invisible
};

// Predefined font sizes
enum class FontSize {
    Large = 72,
    Medium = 48,
    Small = 24
};

// Font descriptor
struct FontDescriptor {
    std::string name;
    std::string filepath;
    int height;
    s8 fontId;
    bool loaded = false;
};

class FontManager {
public:
    static FontManager& Get() {
        static FontManager instance;
        return instance;
    }

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // ========================================================================
    // Font Loading
    // ========================================================================

    // Load a font with custom name, filepath, and height
    // Returns font ID, -1 if failed
    s8 LoadFont(const std::string& name, const std::string& filepath, int height);

    // Load default fonts (Large=72, Medium=48, Small=24)
    void LoadDefaultFonts();

    // Get font ID by name
    s8 GetFontId(const std::string& name) const;

    // Get predefined font IDs (convenience)
    s8 GetLargeFont() const { return m_FontLarge; }
    s8 GetMediumFont() const { return m_FontMedium; }
    s8 GetSmallFont() const { return m_FontSmall; }

    // ========================================================================
    // Text Rendering
    // ========================================================================

    // Print text with left alignment (default behavior)
    // x, y: NDC coordinates
    // scale: text scale
    // r, g, b, a: RGBA color (0-1)
    void Print(s8 fontId, const char* text, f32 x, f32 y, f32 scale, 
               f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print text with specified alignment
    void PrintAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                      TextAlignment alignment,
                      f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print centered text (convenience wrapper)
    void PrintCentered(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                       f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print right-aligned text
    void PrintRight(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                    f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // ========================================================================
    // Theme Support
    // ========================================================================

    // Print with predefined theme
    void PrintTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                    FontTheme theme);

    // Print aligned with theme
    void PrintThemeAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                           TextAlignment alignment, FontTheme theme);

    // Get color from theme
    void GetThemeColor(FontTheme theme, f32& r, f32& g, f32& b, f32& a) const;

    // ========================================================================
    // Multi-line Support
    // ========================================================================

    // Print multi-line text (splits on '\n')
    // lineSpacing: additional space between lines (as multiplier of font height)
    void PrintMultiLine(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                        f32 lineSpacing = 0.1f,
                        f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print multi-line text with alignment
    void PrintMultiLineAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                               TextAlignment alignment, f32 lineSpacing = 0.1f,
                               f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print multi-line with theme
    void PrintMultiLineTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                             FontTheme theme, f32 lineSpacing = 0.1f);

    // ========================================================================
    // Word Wrapping
    // ========================================================================

    // Print wrapped text (word wrap at specified max width in NDC)
    // maxWidth: maximum width in NDC coordinates (e.g., 0.5f)
    void PrintWrapped(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                      f32 maxWidth, f32 lineSpacing = 0.1f,
                      f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print wrapped text with alignment
    void PrintWrappedAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                             TextAlignment alignment, f32 maxWidth, 
                             f32 lineSpacing = 0.1f,
                             f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);

    // Print wrapped with theme
    void PrintWrappedTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                           FontTheme theme, f32 maxWidth, f32 lineSpacing = 0.1f);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Get text dimensions
    void GetTextSize(s8 fontId, const char* text, f32 scale, f32& width, f32& height) const;

    // Get single line height
    f32 GetLineHeight(s8 fontId, f32 scale) const;

    // Unload specific font
    void UnloadFont(const std::string& name);

    // Unload all fonts
    void UnloadAll();

    // Check if font is loaded
    bool IsFontLoaded(s8 fontId) const;

    // ========================================================================
    // Backward Compatibility
    // ========================================================================

    // Legacy global font IDs (for gradual migration)
    s8 g_FontLarge = -1;
    s8 g_FontMedium = -1;
    s8 g_FontSmall = -1;

private:
    FontManager();
    ~FontManager();

    // Helper to split text by newlines
    std::vector<std::string> SplitLines(const char* text) const;

    // Helper to wrap text into lines
    std::vector<std::string> WrapText(s8 fontId, const char* text, f32 scale, f32 maxWidth) const;

    // Theme color lookup
    void SetThemeColors();

    std::map<std::string, s8> m_FontMap;           // name -> fontId
    std::map<s8, FontDescriptor> m_FontDescriptors; // fontId -> descriptor
    s8 m_FontLarge = -1;
    s8 m_FontMedium = -1;
    s8 m_FontSmall = -1;
};

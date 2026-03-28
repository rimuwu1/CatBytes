/* Start Header ************************************************************************/
/*!
\file FontManager.cpp
\author Joash Ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date March 28, 2026
\brief This file implements the FontManager singleton for text rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "FontManager.h"
#include <algorithm>
#include <sstream>

namespace {
// Theme color lookup table
void GetThemeColorImpl(FontTheme theme, f32& r, f32& g, f32& b, f32& a) {
    switch (theme) {
        case FontTheme::White:       r = 1.0f; g = 1.0f; b = 1.0f; a = 1.0f; break;
        case FontTheme::Black:       r = 0.0f; g = 0.0f; b = 0.0f; a = 1.0f; break;
        case FontTheme::Red:         r = 1.0f; g = 0.0f; b = 0.0f; a = 1.0f; break;
        case FontTheme::Green:       r = 0.0f; g = 1.0f; b = 0.0f; a = 1.0f; break;
        case FontTheme::Blue:        r = 0.0f; g = 0.0f; b = 1.0f; a = 1.0f; break;
        case FontTheme::Yellow:      r = 1.0f; g = 1.0f; b = 0.0f; a = 1.0f; break;
        case FontTheme::Cyan:        r = 0.0f; g = 1.0f; b = 1.0f; a = 1.0f; break;
        case FontTheme::Magenta:     r = 1.0f; g = 0.0f; b = 1.0f; a = 1.0f; break;
        case FontTheme::Orange:      r = 1.0f; g = 0.5f; b = 0.0f; a = 1.0f; break;
        case FontTheme::Purple:      r = 0.5f; g = 0.0f; b = 1.0f; a = 1.0f; break;
        case FontTheme::Gold:        r = 1.0f; g = 0.84f; b = 0.0f; a = 1.0f; break;
        case FontTheme::DebugGreen:  r = 0.2f; g = 1.0f; b = 0.3f; a = 1.0f; break;
        case FontTheme::DebugCyan:   r = 0.2f; g = 1.0f; b = 0.35f; a = 1.0f; break;
        case FontTheme::HUDGold:     r = 1.0f; g = 0.95f; b = 0.2f; a = 1.0f; break;
        case FontTheme::Dim:         r = 0.6f; g = 0.6f; b = 0.6f; a = 1.0f; break;
        case FontTheme::Transparent: r = 1.0f; g = 1.0f; b = 1.0f; a = 0.0f; break;
        default:                     r = 1.0f; g = 1.0f; b = 1.0f; a = 1.0f; break;
    }
}
} // namespace

FontManager::FontManager() {
    m_FontLarge = -1;
    m_FontMedium = -1;
    m_FontSmall = -1;
}

FontManager::~FontManager() {
    UnloadAll();
}

// ========================================================================
// Font Loading
// ========================================================================

s8 FontManager::LoadFont(const std::string& name, const std::string& filepath, int height) {
    // Check if already loaded
    auto it = m_FontMap.find(name);
    if (it != m_FontMap.end()) {
        return it->second;
    }

    s8 fontId = AEGfxCreateFont(filepath.c_str(), height);
    if (fontId == -1) {
        return -1;
    }

    FontDescriptor desc;
    desc.name = name;
    desc.filepath = filepath;
    desc.height = height;
    desc.fontId = fontId;
    desc.loaded = true;

    m_FontMap[name] = fontId;
    m_FontDescriptors[fontId] = desc;

    return fontId;
}

void FontManager::LoadDefaultFonts() {
    m_FontLarge = LoadFont("Large", "Assets/Fonts/Lora-Bold.ttf", 72);
    m_FontMedium = LoadFont("Medium", "Assets/Fonts/Lora-Medium.ttf", 48);
    m_FontSmall = LoadFont("Small", "Assets/Fonts/WalterTurncoat-Regular.ttf", 24);

    // Also set legacy globals for backward compatibility during migration
    g_FontLarge = m_FontLarge;
    g_FontMedium = m_FontMedium;
    g_FontSmall = m_FontSmall;
}

s8 FontManager::GetFontId(const std::string& name) const {
    auto it = m_FontMap.find(name);
    if (it != m_FontMap.end()) {
        return it->second;
    }
    return -1;
}

void FontManager::UnloadFont(const std::string& name) {
    auto it = m_FontMap.find(name);
    if (it != m_FontMap.end()) {
        s8 fontId = it->second;
        AEGfxDestroyFont(fontId);
        m_FontDescriptors.erase(fontId);
        m_FontMap.erase(it);
    }
}

void FontManager::UnloadAll() {
    for (auto& pair : m_FontDescriptors) {
        if (pair.second.loaded) {
            AEGfxDestroyFont(pair.first);
            pair.second.loaded = false;
        }
    }
    m_FontMap.clear();
    m_FontDescriptors.clear();
}

bool FontManager::IsFontLoaded(s8 fontId) const {
    return m_FontDescriptors.find(fontId) != m_FontDescriptors.end();
}

// ========================================================================
// Text Rendering
// ========================================================================

void FontManager::Print(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                        f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;
    AEGfxPrint(fontId, text, x, y, scale, r, g, b, a);
}

void FontManager::PrintAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                               TextAlignment alignment, f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;

    f32 width, height;
    GetTextSize(fontId, text, scale, width, height);

    f32 adjustedX = x;
    switch (alignment) {
        case TextAlignment::Center:
            adjustedX = x - width * 0.5f;
            break;
        case TextAlignment::Right:
            adjustedX = x - width;
            break;
        case TextAlignment::Left:
        default:
            // Already at x
            break;
    }

    AEGfxPrint(fontId, text, adjustedX, y, scale, r, g, b, a);
}

void FontManager::PrintCentered(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                f32 r, f32 g, f32 b, f32 a) {
    PrintAligned(fontId, text, x, y, scale, TextAlignment::Center, r, g, b, a);
}

void FontManager::PrintRight(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                             f32 r, f32 g, f32 b, f32 a) {
    PrintAligned(fontId, text, x, y, scale, TextAlignment::Right, r, g, b, a);
}

// ========================================================================
// Theme Support
// ========================================================================

void FontManager::GetThemeColor(FontTheme theme, f32& r, f32& g, f32& b, f32& a) const {
    GetThemeColorImpl(theme, r, g, b, a);
}

void FontManager::PrintTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                             FontTheme theme) {
    f32 r, g, b, a;
    GetThemeColor(theme, r, g, b, a);
    Print(fontId, text, x, y, scale, r, g, b, a);
}

void FontManager::PrintThemeAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                    TextAlignment alignment, FontTheme theme) {
    f32 r, g, b, a;
    GetThemeColor(theme, r, g, b, a);
    PrintAligned(fontId, text, x, y, scale, alignment, r, g, b, a);
}

// ========================================================================
// Multi-line Support
// ========================================================================

std::vector<std::string> FontManager::SplitLines(const char* text) const {
    std::vector<std::string> lines;
    if (!text) return lines;

    std::string str(text);
    std::stringstream ss(str);
    std::string line;

    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    // Handle trailing newline
    if (!str.empty() && str.back() == '\n' && lines.back().empty()) {
        lines.pop_back();
    }

    return lines;
}

void FontManager::PrintMultiLine(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                 f32 lineSpacing, f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;

    std::vector<std::string> lines = SplitLines(text);
    f32 lineHeight = GetLineHeight(fontId, scale);

    f32 currentY = y;
    for (const auto& line : lines) {
        AEGfxPrint(fontId, line.c_str(), x, currentY, scale, r, g, b, a);
        currentY -= lineHeight * (1.0f + lineSpacing);
    }
}

void FontManager::PrintMultiLineAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                         TextAlignment alignment, f32 lineSpacing,
                                         f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;

    std::vector<std::string> lines = SplitLines(text);
    f32 lineHeight = GetLineHeight(fontId, scale);

    f32 currentY = y;
    for (const auto& line : lines) {
        PrintAligned(fontId, line.c_str(), x, currentY, scale, alignment, r, g, b, a);
        currentY -= lineHeight * (1.0f + lineSpacing);
    }
}

void FontManager::PrintMultiLineTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                      FontTheme theme, f32 lineSpacing) {
    f32 r, g, b, a;
    GetThemeColor(theme, r, g, b, a);
    PrintMultiLine(fontId, text, x, y, scale, lineSpacing, r, g, b, a);
}

// ========================================================================
// Word Wrapping
// ========================================================================

std::vector<std::string> FontManager::WrapText(s8 fontId, const char* text, f32 scale, f32 maxWidth) const {
    std::vector<std::string> wrappedLines;
    if (!text || maxWidth <= 0.0f) return wrappedLines;

    std::vector<std::string> lines = SplitLines(text);

    for (const auto& line : lines) {
        if (line.empty()) {
            wrappedLines.push_back("");
            continue;
        }

        std::string currentLine;
        std::istringstream words(line);
        std::string word;

        while (words >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            f32 width, height;
            GetTextSize(fontId, testLine.c_str(), scale, width, height);

            if (width > maxWidth && !currentLine.empty()) {
                wrappedLines.push_back(currentLine);
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }

        if (!currentLine.empty()) {
            wrappedLines.push_back(currentLine);
        }
    }

    return wrappedLines;
}

void FontManager::PrintWrapped(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                f32 maxWidth, f32 lineSpacing, f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;

    std::vector<std::string> wrappedLines = WrapText(fontId, text, scale, maxWidth);
    f32 lineHeight = GetLineHeight(fontId, scale);

    f32 currentY = y;
    for (const auto& line : wrappedLines) {
        AEGfxPrint(fontId, line.c_str(), x, currentY, scale, r, g, b, a);
        currentY -= lineHeight * (1.0f + lineSpacing);
    }
}

void FontManager::PrintWrappedAligned(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                       TextAlignment alignment, f32 maxWidth, f32 lineSpacing,
                                       f32 r, f32 g, f32 b, f32 a) {
    if (!text || !IsFontLoaded(fontId)) return;

    std::vector<std::string> wrappedLines = WrapText(fontId, text, scale, maxWidth);
    f32 lineHeight = GetLineHeight(fontId, scale);

    f32 currentY = y;
    for (const auto& line : wrappedLines) {
        PrintAligned(fontId, line.c_str(), x, currentY, scale, alignment, r, g, b, a);
        currentY -= lineHeight * (1.0f + lineSpacing);
    }
}

void FontManager::PrintWrappedTheme(s8 fontId, const char* text, f32 x, f32 y, f32 scale,
                                     FontTheme theme, f32 maxWidth, f32 lineSpacing) {
    f32 r, g, b, a;
    GetThemeColor(theme, r, g, b, a);
    PrintWrapped(fontId, text, x, y, scale, maxWidth, lineSpacing, r, g, b, a);
}

// ========================================================================
// Utility Functions
// ========================================================================

void FontManager::GetTextSize(s8 fontId, const char* text, f32 scale, f32& width, f32& height) const {
    width = 0.0f;
    height = 0.0f;

    if (!text || !IsFontLoaded(fontId)) return;

    AEGfxGetPrintSize(fontId, text, scale, &width, &height);
}

f32 FontManager::GetLineHeight(s8 fontId, f32 scale) const {
    f32 width, height;
    GetTextSize(fontId, "Ay", scale, width, height);
    return height;
}

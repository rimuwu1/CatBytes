/* Start Header ************************************************************************/
/*!
\file Fonts.h
\author Sim Hui Min, s.huimin, 2503506
\par s.huimin@digipen.edu
\date January, 24, 2026
\brief This file declares global fonts used throughout the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#ifndef FONTS_H
#define FONTS_H

// Include the new FontManager for full functionality
#include "FontManager.h"

// ========================================================================
// Backward Compatibility Layer
// ========================================================================
// These are now forwarded to FontManager for gradual migration
// Use FontManager::Get() for new code

// Legacy global font IDs - now pointing to FontManager
// Usage: FontManager::Get().g_FontLarge, etc.
// Or use: FontManager::Get().GetLargeFont(), GetMediumFont(), GetSmallFont()
#define g_FontLarge FontManager::Get().g_FontLarge
#define g_FontMedium FontManager::Get().g_FontMedium
#define g_FontSmall FontManager::Get().g_FontSmall

// Legacy function names - now forwarded to FontManager
inline void Fonts_Load() { FontManager::Get().LoadDefaultFonts(); }
inline void Fonts_Unload() { FontManager::Get().UnloadAll(); }

#endif

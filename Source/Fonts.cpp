/* Start Header ************************************************************************/
/*!
\file Fonts.cpp
\author Sim Hui Min, s.huimin, 2503506
\par s.huimin@digipen.edu
\date January, 24, 2026
\brief This file implements global font loading and management.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "Fonts.h"

// global font vairables
s8 g_FontLarge = -1;
s8 g_FontMedium = -1;
s8 g_FontSmall = -1;

void Fonts_Load()
{
    g_FontLarge = AEGfxCreateFont("../../Extern/Fonts/Lora-Bold.ttf", 72);
    g_FontMedium = AEGfxCreateFont("../../Extern/Fonts/Lora-Medium.ttf", 48);
    g_FontSmall = AEGfxCreateFont("../../Extern/Fonts/WalterTurncoat-Regular.ttf", 24);
}

void Fonts_Unload()
{
    if (g_FontLarge != -1) AEGfxDestroyFont(g_FontLarge);
    if (g_FontMedium != -1) AEGfxDestroyFont(g_FontMedium);
    if (g_FontSmall != -1) AEGfxDestroyFont(g_FontSmall);
}
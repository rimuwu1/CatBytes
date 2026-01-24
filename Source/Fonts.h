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

#include "AEEngine.h"

// global font IDs
extern s8 g_FontLarge;   
extern s8 g_FontMedium; 
extern s8 g_FontSmall;  

void Fonts_Load(); // initialize 

void Fonts_Unload(); // destroy

#endif
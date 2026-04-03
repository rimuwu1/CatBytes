/* Start Header ************************************************************************/
/*!
\file Controls.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date January, 24, 2026
\brief Declares functions and state used for the Controls menu and in-game
controls overlay, including initialization, update, rendering, and cleanup.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

//check if controls is called from pause menu
extern bool g_FromPause;

// Initialization and cleanup
void Controls_Load();
void Controls_Initialize();
void Controls_Free();
void Controls_Unload();

// Update
void Controls_Update();
void Controls_UpdateOverlay();

// Rendering
void Controls_Draw();
void Controls_DrawOverlay(float camX, float camY);
void DrawKeysPage();
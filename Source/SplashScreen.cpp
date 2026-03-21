/* Start Header ************************************************************************/
/*!
\file SplashScreen.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements the credit sequence in the beginning of gamestate.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "GameStateManager.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include <cmath>
#include "Fonts.h"

constexpr float CYCLE_DURATION = 4.0f;
constexpr int   PHASES = 4;
constexpr float PHASE_DURATION = CYCLE_DURATION / PHASES;  // 1.0f
constexpr float MAX_OPACITY = 1.0f;

AEGfxTexture* Digipen;
AEGfxTexture* Logo;

int phase;
float x, y, dt, t, opacity, phaseProgress, DigipenW, DigipenH, LogoW, LogoH;
static float totalElapsedTime{};
void SplashScreen_Load()
{
	Digipen = TextureManager::Get().LoadTexture("Assets/Images/DigiPen_BLACK.png");
	Logo = TextureManager::Get().LoadTexture("Assets/Images/titleicon.png");
	std::cout << "Splash Screen: Load" << std::endl;
}

void SplashScreen_Initialize()
{
    DigipenW = 1525.0f / 2.0f;
    DigipenH = 445.0f / 2.0f;
	LogoW = 1600.0f / 2.0f;
	LogoH = 900.0f / 2.0f;
    x = 0.0f;
	y = 0.0f;
	
	std::cout << "Splash Screen: Initialize" << std::endl;

}

void SplashScreen_Update()
{
	dt = static_cast<float>(AEFrameRateControllerGetFrameTime());
	totalElapsedTime += dt;
	/* For drawing, clamp time to the cycle so we can render the final fade-out frame.
	   Use unclamped totalElapsedTime to decide when to transition to the next state. */
	t = totalElapsedTime;
	if (t > CYCLE_DURATION) t = CYCLE_DURATION;

	phase = static_cast<int>(floorf(t / PHASE_DURATION)); //get phase based on time
	if (phase >= PHASES) phase = PHASES - 1;
	phaseProgress = (t - phase * PHASE_DURATION) / PHASE_DURATION; //phase progress from 0.0 to 1.0

	opacity = 0.0f;
	std::cout << "Splash Screen: Update" << std::endl;
}

void SplashScreen_Draw()
{
	AESysFrameStart();
	  
    /* Phases:
        0 - Digipen fade in
        1 - Digipen fade out
        2 - Logo fade in
        3 - Logo fade out
    */
    if (phase == 0) {
        /* Digipen fade in */
        opacity = phaseProgress * MAX_OPACITY;
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        MeshManager::Get().DrawTexturedSquare(Digipen, x, y, DigipenW, DigipenH, opacity);
        AEGfxPrint(g_FontLarge, "All content (c) 2026 DigiPen Institute of Technology Singapore. All Rights Reserved. ", -0.4f, -0.7f, 0.25f, 1.0f, 1.0f, 1.0f, opacity);
      
    }
    else if (phase == 1) {
        /* Digipen fade out */
        opacity = (1.0f - phaseProgress) * MAX_OPACITY;
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        MeshManager::Get().DrawTexturedSquare(Digipen, x, y, DigipenW, DigipenH, opacity);
        AEGfxPrint(g_FontLarge, "All content (c) 2026 DigiPen Institute of Technology Singapore. All Rights Reserved. ", -0.4f, -0.7f, 0.25f, 1.0f, 1.0f, 1.0f, opacity);
    }
    else if (phase == 2) {
        /* Logo fade in */
        opacity = phaseProgress * MAX_OPACITY;
        AEGfxSetBackgroundColor( 0.9f, 0.9f, 0.9f);
        MeshManager::Get().DrawTexturedSquare(Logo, x, y, LogoW, LogoH, opacity);
    }
    else {
        /* Logo fade out */
        opacity = (1.0f - phaseProgress) * MAX_OPACITY;
        AEGfxSetBackgroundColor(0.9f, 0.9f, 0.9f);
        MeshManager::Get().DrawTexturedSquare(Logo, x, y, LogoW, LogoH, opacity);
    }
    

    /* Transition to main menu after the full cycle finished (after phase 3 completes) */
    if (totalElapsedTime >= CYCLE_DURATION) {
		GameStateManager::Get().next = GS_MAINMENU;
    }
	AESysFrameEnd();
	std::cout << "Splash Screen: Draw" << std::endl;
}

void SplashScreen_Free()
{
	std::cout << "Splash Screen: Free" << std::endl;
}

void SplashScreen_Unload()
{
	std::cout << "Splash Screen: Unload" << std::endl;
}
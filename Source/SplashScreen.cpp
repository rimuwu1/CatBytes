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
#include "MainGame.h"
#include "TransitionManager.h"

constexpr float CYCLE_DURATION = 4.0f;
constexpr int   PHASES = 4;
constexpr float PHASE_DURATION = CYCLE_DURATION / PHASES;  // 1.0f
constexpr float MAX_OPACITY = 1.0f;

AEGfxTexture* Digipen;
AEGfxTexture* Logo;
AEGfxTexture* meleeIcon;

int phase;
float x, y, dt, t, opacity, phaseProgress, DigipenW, DigipenH, LogoW, LogoH;
float meleeIconW, meleeIconH;
static float totalElapsedTime{};
void SplashScreen_Load()
{
	Digipen = TextureManager::Get().LoadTexture("Assets/Images/DigiPen_BLACK.png");
	Logo = TextureManager::Get().LoadTexture("Assets/Images/titleicon.png");
	meleeIcon = TextureManager::Get().LoadTexture("Assets/Images/meleeIcon.png");
	std::cout << "Splash Screen: Load" << std::endl;
	TransitionManager::GetInstance().Init();
}

void SplashScreen_Initialize()
{
    DigipenW = 1525.0f / 2.0f;
    DigipenH = 445.0f / 2.0f;
	LogoW = 1600.0f / 2.0f;
	LogoH = 900.0f / 2.0f;
    x = 0.0f;
	y = 0.0f;
	meleeIconW = 200.0f;
	meleeIconH = 200.0f;
	totalElapsedTime = 0.0f;  // Reset timer for fresh splash screen
	
	// Start async JSON parsing in background during splash screen
	StartAsyncLoading();
	
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

	TransitionManager::GetInstance().Update(dt);
}

void SplashScreen_Draw()
{
	AESysFrameStart();
	  
    /* During cutscene (first 4 seconds): play animation
       After cutscene: show loading screen
    */
    if (totalElapsedTime < CYCLE_DURATION) {
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
            FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "All content (c) 2026 DigiPen Institute of Technology Singapore. All Rights Reserved. ", 0.0f, -0.7f, 0.25f, 1.0f, 1.0f, 1.0f, opacity);
        
        }
        else if (phase == 1) {
            /* Digipen fade out */
            opacity = (1.0f - phaseProgress) * MAX_OPACITY;
            AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
            MeshManager::Get().DrawTexturedSquare(Digipen, x, y, DigipenW, DigipenH, opacity);
            FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "All content (c) 2026 DigiPen Institute of Technology Singapore. All Rights Reserved. ", 0.0f, -0.7f, 0.25f, 1.0f, 1.0f, 1.0f, opacity);
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
        
        // "ESC to skip" hint in top left during cutscene only
        FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(),
            "ESC",
            -0.80f, 0.88f, 0.6f,
            0.5f, 0.4f, 0.75f, 0.5f);   // teal, more transparent

        FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(),
            "to skip",
            -0.68f, 0.88f, 0.6f,
            0.5f, 0.5f, 0.5f, 0.4f);    // grey, more transparent
    }
    else {
        // After cutscene: show loading screen
        AEGfxSetBackgroundColor(0.9f, 0.9f, 0.9f);  
        
        // Do incremental loading on main thread (one step per frame)
        bool loadingDone = ContinueMainThreadLoading();
        
        // Get loading progress
        int pct = GetLoadingProgress();
        
        // Draw meleeIcon centered in middle
        MeshManager::Get().DrawTexturedSquare(meleeIcon, 0.0f, 100.0f, meleeIconW, meleeIconH, 1.0f);
        
        // Loading bar dimensions
        float barWidth = 400.0f;
        float barHeight = 20.0f;
        float barY = -50.0f;
        float fillWidth = barWidth * (pct / 100.0f);
        
        // Draw bar background (dark purple)
        MeshManager::Get().DrawSquare(0.0f, barY, barWidth, barHeight, 86, 0, 137, 200);
        
        // Draw filled portion (brighter purple)
        if (fillWidth > 0) {
            MeshManager::Get().DrawSquare(-barWidth * 0.5f + fillWidth * 0.5f, barY, fillWidth, barHeight, 197, 15, 255, 255);
        }
        
        // Show loading progress text below bar
        char buf[64];
        snprintf(buf, sizeof(buf), "Loading Resources... %d%%", pct);
        FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), 
            buf, 0.0f, -0.3f, 0.4f, 
            1.0f, 1.0f, 1.0f, 1.0f);
        
        // Transition to main menu when loading complete
        if (loadingDone) {
            TransitionManager::GetInstance().Start(GS_MAINMENU);
        }
    }
    
	AESysFrameEnd();
	std::cout << "Splash Screen: Draw" << std::endl;

	TransitionManager::GetInstance().Draw();
}

void SplashScreen_Free()
{
	std::cout << "Splash Screen: Free" << std::endl;
}

void SplashScreen_Unload()
{
	std::cout << "Splash Screen: Unload" << std::endl;
}

void SplashScreen_SkipCutscene()
{
	// Jump to loading phase (cutscene ends at CYCLE_DURATION)
	totalElapsedTime = CYCLE_DURATION;
}

bool SplashScreen_IsInCutscene()
{
	// Cutscene is still playing if we haven't reached loading phase yet
	return totalElapsedTime < CYCLE_DURATION;
}
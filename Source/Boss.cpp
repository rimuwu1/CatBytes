/* Start Header ************************************************************************/
/*!
\file       Level4.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 24 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Boss.h"
#include "Background.h"
#include "LevelIndicator.h"

// ----------------------------------------------------------------------------
// Loads Level 2 resources and initial data
// Reads the initial number of lives from a text file
// ----------------------------------------------------------------------------
void Boss_Load()
{
	
}

// ----------------------------------------------------------------------------
// Initializes Level 2 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Boss_Initialize()
{
	
	// initialise level indicator
	LevelIndicator_Initialize();

}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Boss_Update()
{

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	float dt = (float)AEFrameRateControllerGetFrameTime();

	// Background Update
	const float camSpeed = 100.0f;

	// !! MANUAL KEYBOARD INPUT FOR CAM; TO BE REMOVED ONCE CAM IS IN !!
		// W key: up, S key: down
	if (AEInputCheckCurr(AEVK_UP)) {

		debugCamY += camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_DOWN)) {

		debugCamY -= camSpeed * dt;

	}

	Background_Update(debugCamY);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection == 3 && currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// update when section changes
	LevelIndicator_Update(dt);

}

// ----------------------------------------------------------------------------
// Renders Level 2 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Boss_Draw()
{

	// Informing the system about the loop's start
	AESysFrameStart();

	// draw background
	Background_Draw();

	// draw text for level indicator
	LevelIndicator_Draw();

	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 2
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Boss_Free()
{
	
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Boss_Unload()
{
	
}
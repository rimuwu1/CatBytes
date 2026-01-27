/* Start Header ************************************************************************/
/*!
\file       Level3.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 24 2026
\brief		This file implements the functions for Level 3 of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Level3.h"
#include "Background.h"
#include "LevelIndicator.h"
#include "Platforms.h"
#include "Camera.h"
#include "Player.h"
#include "Utils.h"

static Player lv3Player;

AEGfxVertexList* lv3mesh;

// platforms array
static std::vector<Platform> level3Platforms = {
	{  500.0f,  999.0f, 300.0f, 40.0f }, // level 2's last platform
	{  500.0f,  999.0f, 300.0f, 40.0f }
};

// ----------------------------------------------------------------------------
// Loads Level 2 resources and initial data
// Reads the initial number of lives from a text file
// ----------------------------------------------------------------------------
void Level3_Load()
{
	
}

// ----------------------------------------------------------------------------
// Initializes Level 2 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level3_Initialize()
{
	
	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise camera
	Camera_Init(globalCam, lv3Player.pos.x, lv3Player.pos.y);

}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Level3_Update()
{

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	float dt = (float)AEFrameRateControllerGetFrameTime();

	// toggle use debug cam
		if (AEInputCheckTriggered(AEVK_1)) {

			globalCam.debugCam = !globalCam.debugCam;

		}

	if (globalCam.debugCam) {

		Camera_Debug(globalCam, dt);

	}
	else {

		// camera follows player
		Camera_FollowPlayer(globalCam, lv3Player.pos.x, lv3Player.pos.y, dt);

		// apply camera
		Camera_Apply(globalCam);

	}

	// update background based on y axis
	Background_Update(globalCam.y);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection == 2 && currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// exit level 3 & goes to boss level
	const float endOfLevel3 = sectionHeight[2];

	if (globalCam.y >= endOfLevel3) {

		next = GS_LEVEL4;

	}


	// update when section changes
	LevelIndicator_Update(dt);

}

// ----------------------------------------------------------------------------
// Renders Level 2 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Level3_Draw()
{
	
	// Informing the system about the loop's start
	AESysFrameStart();

	// draw background
	Background_Draw();

	// draw text for level indicator
	LevelIndicator_Draw();

	// draw platforms
	Platforms_Draw(lv3mesh, level3Platforms);

	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 2
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level3_Free()
{
	
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level3_Unload()
{
	
}
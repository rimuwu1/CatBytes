/* Start Header ************************************************************************/
/*!
\file Level2.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Level 2 of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Level2.h"
#include "Background.h"
#include "LevelIndicator.h"
#include "Platforms.h"
#include "Utils.h"

// Global variables for tracking Level 2 game state
int Level2_Counter;  // Tracks remaining updates before checking lives
int Level2_Lives;    // Tracks remaining player lives in Level 2

AEGfxVertexList* lv2mesh;

// platforms array
static std::vector<Platform> level2Platforms = {
	{ 0.0f, 600.0f, 300.0f, 40.0f },
	
};

// ----------------------------------------------------------------------------
// Loads Level 2 resources and initial data
// Reads the initial number of lives from a text file
// ----------------------------------------------------------------------------
void Level2_Load()
{
	// Log that loading is complete
	std::cout << "Level2:Load" << std::endl;
}

// ----------------------------------------------------------------------------
// Initializes Level 2 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level2_Initialize()
{
	lv2mesh = util::CreateSquareMesh();

	// reset background
	Background_Reset(sectionHeight[0], 1);

	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise platforms
	//Platforms_Initialize();

	// Log that initialization is complete
	std::cout << "Level2:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Level2_Update()
{
	std::cout << "Level2:Update" << std::endl;

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	float dt = (float)AEFrameRateControllerGetFrameTime();

	// Background Update
	const float camSpeed = 800.0f;

	// !! MANUAL KEYBOARD INPUT FOR CAM; TO BE REMOVED ONCE CAM IS IN !!
		// W key: up, S key: down
	if (AEInputCheckCurr(AEVK_UP)) {

		debugCamY += camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_DOWN)) {

		debugCamY -= camSpeed * dt;

	}

	Background_Update(debugCamY);

	AEGfxSetCamPosition(0.0f, debugCamY);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection == 1 && currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// exit level 2 & goes to level 3
	float lvl2HighestPlatform = level2Platforms[0].y + level2Platforms[0].h / 2;

	for (const auto& pf : level2Platforms) {

		float platformMax = pf.y + pf.h / 2;

		if (platformMax > lvl2HighestPlatform) {

			lvl2HighestPlatform = platformMax;

		}

	}
	const float endOfLevel2 = sectionHeight[1];

	if (debugCamY >= endOfLevel2) {

		next = GS_LEVEL3;

	}

	// update when section changes
	LevelIndicator_Update(dt);
}

// ----------------------------------------------------------------------------
// Renders Level 2 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Level2_Draw()
{
	std::cout << "Level2:Draw" << std::endl;

	// Informing the system about the loop's start
	AESysFrameStart();

	// draw background
	Background_Draw();

	// draw text for level indicator
	LevelIndicator_Draw();

	// draw platforms
	Platforms_Draw(lv2mesh,level2Platforms);

	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 2
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level2_Free()
{
	std::cout << "Level2:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level2_Unload()
{
	AEGfxMeshFree(lv2mesh);
	std::cout << "Level2:Unload" << std::endl;
}
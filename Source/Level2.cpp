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
#include "Camera.h"
#include "Player.h"
#include "Utils.h"

static Player lv2Player;

AEGfxVertexList* lv2mesh;

// platforms array
static std::vector<Platform> level2Platforms = {
	{ -300.0f,	450.0f, 520.0f, 40.0f }, // level's 1 last platform
	{  255.0f,  575.0f, 670.0f, 40.0f },
	{ -280.0f,  695.0f, 300.0f, 40.0f },
	{  650.0f,  695.0f, 200.0f, 40.0f },
	{  190.0f,  840.0f, 500.0f, 40.0f },
	{ -190.0f,  950.0f, 200.0f, 40.0f },
	{  500.0f,  999.0f, 300.0f, 40.0f }
	
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

	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise platforms
	//Platforms_Initialize();

	// initialise camera
	Camera_Init(globalCam, lv2Player.pos.x, lv2Player.pos.y);

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

	// toggle use debug cam
	if (AEInputCheckTriggered(AEVK_1)) {

		globalCam.debugCam = !globalCam.debugCam;

	}

	if (globalCam.debugCam) {

		Camera_Debug(globalCam);

	}
	else {

		// camera follows player
		Camera_FollowPlayer(globalCam, lv2Player.pos.x, lv2Player.pos.y, dt);

		// apply camera
		Camera_Apply(globalCam);

	}

	// update background based on y axis
	Background_Update(globalCam.y);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection == 1 && currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// exit level 2 & goes to level 3
	const float endOfLevel2 = sectionHeight[1];

	if (globalCam.y >= endOfLevel2) {

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
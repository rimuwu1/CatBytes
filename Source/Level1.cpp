/* Start Header ************************************************************************/
/*!
\file Level1.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Level 1 of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Level1.h"
#include "Player.h"
#include "Utils.h"
#include "Input.h"

static Player lv1Player;
AEGfxVertexList* lvl1mesh;

float ground;

// ----------------------------------------------------------------------------
// Loads Level 1 resources and initial data
// Reads the Level1_Counter value from a text file to determine level duration
// ----------------------------------------------------------------------------
void Level1_Load()
{
	std::cout << "Level1:Load" << std::endl;
}

// ----------------------------------------------------------------------------
// Initializes Level 1 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level1_Initialize()
{
	// Player Initialization
	lvl1mesh = util::CreateSquareMesh();
	ground = -350.0f;
	Player_Init(lv1Player, 0.0f, ground);
	lv1Player.grounded = 1;

	// Bind the level player to the input system
	Input_SetPlayer(&lv1Player);

	std::cout << "Level1:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates Level 1 logic every frame
// Decrements the counter and checks for level completion
// ----------------------------------------------------------------------------
void Level1_Update()
{
	AEGfxSetBackgroundColor(.7f, .7f, .7f);
	
	// Player Update
	float dt = (float)AEFrameRateControllerGetFrameTime();
	Player_Update(lv1Player, dt);
	//lv1Player.grounded = 0;

	if (lv1Player.pos.y - lv1Player.height <= ground)
	{
		lv1Player.pos.y = ground + lv1Player.height;
		lv1Player.vel.y = 0.0f;
		lv1Player.grounded = 1;
	}
	std::cout << "Level1:Update" << std::endl;
	
}

// ----------------------------------------------------------------------------
// Renders Level 1 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Level1_Draw()
{
	// Informing the system about the loop's start
	AESysFrameStart();
	util::DrawSquare(lvl1mesh, 0.0f, ground, 1600.0f, 50.0f, 0, 0, 0); // Draw Ground (Texture TBA?)
	Player_Draw(lv1Player);
	std::cout << "Level1:Draw" << std::endl;
	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 1
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level1_Free()
{
	std::cout << "Level1:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 1 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level1_Unload()
{
	AEGfxMeshFree(lvl1mesh);
	std::cout << "Level1:Unload" << std::endl;
}
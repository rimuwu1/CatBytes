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
#include "enemy.h"//Enemy
#include "Background.h"
#include "LevelIndicator.h"

static Player lv1Player;
static Enemy EasyEnemy; //Enemy
AEGfxVertexList* lvl1mesh;

float ground;

// ----------------------------------------------------------------------------
// Loads Level 1 resources and initial data
// Reads the Level1_Counter value from a text file to determine level duration
// ----------------------------------------------------------------------------
void Level1_Load()
{
	
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

	//enemy Initialization
	Enemy_Init(EasyEnemy, 200.0f, ground + 50.0f);//Enemy

	std::cout << "Level1:Initialize" << std::endl;
	// !! remove once cam in
	fakeCamY = 0.0f;

	// initialise background
	Background_Initialise();

	// initialise level indicator
	LevelIndicator_Initialize();
}

// ----------------------------------------------------------------------------
// Updates Level 1 logic every frame
// Decrements the counter and checks for level completion
// ----------------------------------------------------------------------------
void Level1_Update()
{
	AEGfxSetRenderMode(AE_GFX_RM_COLOR); //to render colours (change if using texture)
	
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

	//enemy update
	Enemy_Update(EasyEnemy, dt);//Enemy

	//Enemy
	// TEMPORARY collision test
	float collisionDistX = lv1Player.width / 2 + EasyEnemy.width / 2;
	float collisionDistY = lv1Player.height / 2 + EasyEnemy.height / 2;

	bool currentlyColliding =
		fabs(lv1Player.pos.x - EasyEnemy.pos.x) < collisionDistX &&
		fabs(lv1Player.pos.y - EasyEnemy.pos.y) < collisionDistY;

	if (currentlyColliding && !EasyEnemy.isPlayerColliding)
	{
		Enemy_OnHit(EasyEnemy); // hit only once per collision
		EasyEnemy.isPlayerColliding = true;
	}
	else if (!currentlyColliding)
	{
		EasyEnemy.isPlayerColliding = false; //reset for next collision
	}
	//Enemy
	
	// Background Update
	const float camSpeed = 100.0f;

	// !! MANUAL KEYBOARD INPUT FOR CAM; TO BE REMOVED ONCE CAM IS IN !!
		// W key: up, S key: down
	if (AEInputCheckCurr(AEVK_W)) {

		fakeCamY += camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_S)) {

		fakeCamY -= camSpeed * dt;

	}

	Background_Update(fakeCamY);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection == 0 && currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// exit level 1 & goes to level 2
	const float endOfLevel1 = sectionHeight[0];

	if (fakeCamY >= endOfLevel1) {

		next = GS_LEVEL2;

	}

	// update when section changes
	LevelIndicator_Update(dt);

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
	Enemy_Draw(EasyEnemy);//Enemy

	// draw background
	Background_Draw();

	util::DrawSquare(squareMesh, 0.0f, ground, 1600.0f, 50.0f, 0, 0, 0); // Draw Ground (Texture TBA?)
	Player_Draw(lv1Player);

	// draw text for level indicator
	LevelIndicator_Draw();

	AESysFrameEnd();

}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 1
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level1_Free()
{
	Enemy_Free();//Enemy
	
}

// ----------------------------------------------------------------------------
// Unloads Level 1 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level1_Unload()
{
	AEGfxMeshFree(lvl1mesh);
	
}
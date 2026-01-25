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

static Player lv1Player;
static Enemy EasyEnemy; //Enemy
AEGfxVertexList* lv1mesh;

float ground;

struct Platform {
	float x, y, w, h;
};

static Platform platforms[] = {
	{ -400.0f, -200.0f, 300.0f, 40.0f },
	{ -250.0f, -120.0f, 250.0f, 40.0f },
	{ -150.0f,	 20.0f, 220.0f, 40.0f }
};

static const int platformCount = sizeof(platforms) / sizeof(platforms[0]);

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
	lv1mesh = util::CreateSquareMesh();
	ground = -350.0f;
	Player_Init(lv1Player, 0.0f, ground);
	lv1Player.grounded = 1;

	// Bind the level player to the input system
	Input_SetPlayer(&lv1Player);

	//enemy Initialization
	Enemy_Init(EasyEnemy, 200.0f, ground + 50.0f);//Enemy

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
	static float playerPrevY = 0.0f;
	playerPrevY = lv1Player.pos.y;

	float dt = (float)AEFrameRateControllerGetFrameTime();
	Player_Update(lv1Player, dt);
	//lv1Player.grounded = 0;

	if (lv1Player.pos.y - lv1Player.height <= ground)
	{
		lv1Player.pos.y = ground + lv1Player.height;
		lv1Player.vel.y = 0.0f;
		lv1Player.grounded = 1;

		playerPrevY = lv1Player.pos.y;
	}

	if (lv1Player.vel.y <= 0.0f)
	{
		float playerPrevBottom = playerPrevY - lv1Player.height;
		float playerCurrBottom = lv1Player.pos.y - lv1Player.height;

		for (int i = 0; i < platformCount; ++i)
		{
			const Platform& pf = platforms[i];

			float pfLeft = pf.x - pf.w * 0.5f;
			float pfRight = pf.x + pf.w * 0.5f;

			float playerLeft = lv1Player.pos.x - lv1Player.width * 0.5f;
			float playerRight = lv1Player.pos.x + lv1Player.width * 0.5f;

			bool overlapX = (playerRight >= pfLeft) && (playerLeft <= pfRight);

			bool crossedTop = (playerPrevBottom >= pf.y) && (playerCurrBottom <= pf.y);

			if (overlapX && crossedTop)
			{
				lv1Player.pos.y = pf.y + lv1Player.height;
				lv1Player.vel.y = 0.0f;
				lv1Player.grounded = 1;

				playerPrevY = lv1Player.pos.y;

				break;
			}

		}

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

	// Draw platforms
	for (int i = 0; i < platformCount; ++i)
	{
		util::DrawSquare(
			lv1mesh,
			platforms[i].x,
			platforms[i].y - platforms[i].h * 0.5f,
			platforms[i].w,
			platforms[i].h,
			60, 60, 60
		);
	}

	util::DrawSquare(lv1mesh, 0.0f, ground, 1600.0f, 50.0f, 0, 0, 0); // Draw Ground (Texture TBA?)
	Player_Draw(lv1Player);
	Enemy_Draw(EasyEnemy);//Enemy
	std::cout << "Level1:Draw" << std::endl;
	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 1
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level1_Free()
{
	Enemy_Free();//Enemy
	std::cout << "Level1:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 1 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level1_Unload()
{
	AEGfxMeshFree(lv1mesh);
	std::cout << "Level1:Unload" << std::endl;
}
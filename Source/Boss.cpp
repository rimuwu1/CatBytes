/* Start Header ************************************************************************/
/*!
\file       Boss.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 24 2026
\brief		This file implements the functions for the game's boss level.

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
#include "Player.h"
#include "Camera.h"
#include "Platforms.h"
#include "enemy.h"
#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"

//extern rapidjson::Document level4Config;
extern rapidjson::Document level1Config;

static Player bossPlayer;

static Enemy bossEnemy;
static AEGfxVertexList* bossMesh = nullptr;
static AEGfxTexture* bossTexture = nullptr;

// platforms array
static std::vector<Platform> bossPlatforms = {
	{  600.0f,  1650.0f, 250.0f, 40.0f },
	{ -600.0f,  1650.0f, 250.0f, 40.0f },
	{    0.0f,  1800.0f, 1000.0f, 40.0f }
};

// -----------------------------------------------------------------------------
// Initialize the boss enemy
// Loads position, HP, speed, and damage from level_4 config; creates mesh and texture
// -----------------------------------------------------------------------------
void BossEnemy_Init(Enemy& enemy, float startX, float startY)
{
	enemy.pos = { startX, startY };
	enemy.width = 120.0f;   // boss is bigger than regular enemies
	enemy.height = 120.0f;

	// load speed from config; fallback to 120
	enemy.moveSpeed = level1Config["level_4"]["enemies"][0]["speed"].GetFloat();
	if (enemy.moveSpeed <= 0.0f) enemy.moveSpeed = 120.0f;

	// load HP from config; fallback to 30
	enemy.hitPoints = level1Config["level_4"]["enemies"][0]["hp"].GetFloat();
	if (enemy.hitPoints <= 0.0f) enemy.hitPoints = 30.0f;

	// load collision damage from config; fallback to 8
	enemy.damage = level1Config["level_4"]["enemies"][0]["damage"].GetFloat();
	if (enemy.damage <= 0.0f) enemy.damage = 8.0f;

	enemy.shootCooldown = 0.0f;
	enemy.shootTimer = 0.0f;
	enemy.direction = 1;     // start moving right
	enemy.isAlive = 1;
	enemy.hitStunTimer = 0.0f;
	enemy.isPlayerColliding = false;

	// create mesh if needed
	if (!bossMesh)
		bossMesh = util::CreateSquareMesh();

	// load boss texture if not already loaded
	if (!enemy.texture)
		enemy.texture = AEGfxTextureLoad("Assets/Images/Boss.jpg");
}

// -----------------------------------------------------------------------------
// Update boss enemy each frame
// Patrols left and right within a wider range than regular enemies
// Freezes briefly on hit stun
// -----------------------------------------------------------------------------
void BossEnemy_Update(Enemy& enemy, float dt)
{
	if (!enemy.isAlive) return;

	// handle hit stun (freeze)
	if (enemy.hitStunTimer > 0.0f)
	{
		enemy.hitStunTimer -= dt;
		if (enemy.hitStunTimer <= 0.0f)
			enemy.hitStunTimer = 0.0f;
		return; // do not move while stunned
	}

	// patrol movement
	enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;

	// wider patrol bounds for the boss arena
	float patrolMinX = -400.0f;
	float patrolMaxX = 400.0f;

	if (enemy.pos.x >= patrolMaxX)
		enemy.direction = -1;   // move left
	else if (enemy.pos.x <= patrolMinX)
		enemy.direction = 1;    // move right
}

// -----------------------------------------------------------------------------
// Draw the boss enemy on screen
// -----------------------------------------------------------------------------
void BossEnemy_Draw(const Enemy& enemy)
{
	if (!enemy.isAlive) return;

	// flip horizontally when moving right (original image faces left)
	float scaleX = (enemy.direction == 1) ? -enemy.width : enemy.width;

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	util::DrawTexturedSquare(
		bossMesh,
		enemy.texture,
		enemy.pos.x,
		enemy.pos.y,
		scaleX,
		enemy.height,
		1.0f
	);
}

// -----------------------------------------------------------------------------
// Free boss-specific static resources (mesh and texture)
// -----------------------------------------------------------------------------
void BossEnemy_Free()
{
	if (bossEnemy.texture)
	{
		AEGfxTextureUnload(bossEnemy.texture);
		bossEnemy.texture = nullptr;
	}

	if (bossMesh)
	{
		AEGfxMeshFree(bossMesh);
		bossMesh = nullptr;
	}
}

// collision damage
void BossEnemy_OnCollision(Enemy& enemy, Player& player)
{
	if (!enemy.isAlive || enemy.hitStunTimer > 0.0f)
		return;

	next = GS_WINLOSE;
}


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

	// initialise camera
	Camera_Init(globalCam, bossPlayer.pos.x, bossPlayer.pos.y);

}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Boss_Update()
{

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	float dt = (float)AEFrameRateControllerGetFrameTime();

	// toggle use debug cam
	if (AEInputCheckTriggered(AEVK_1)) {

		Camera_Debug(globalCam, dt);

	}

	// camera follows player
	Camera_FollowPlayer(globalCam, bossPlayer.pos.x, bossPlayer.pos.y, dt);

	// apply camera
	Camera_Apply(globalCam);

	// update background based on y axis
	Background_Update(globalCam.y);

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
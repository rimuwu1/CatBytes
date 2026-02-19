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
#include "MeshManager.h"
#include "TextureManager.h"
#include "FileManager.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include "enemy.h"

//extern rapidjson::Document level4Config;
extern rapidjson::Document level1Config;

static Player bossPlayer;

//AEGfxVertexList* bossPlatformMesh;

rapidjson::Document bossConfig;

std::ifstream ifs4;

static Enemy bossEnemy;
static AEGfxTexture* bossTexture = nullptr;

// platforms array
static std::vector<Platform> bossPlatforms = {
	{  600.0f,  1650.0f, 250.0f, 40.0f },
	{ -600.0f,  1650.0f, 250.0f, 40.0f },
	{    0.0f,  1800.0f, 1000.0f, 40.0f }
};

// platforms array - loaded from JSON
//std::vector<Platform> bossPlatforms;

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

	// load boss texture if not already loaded
	if (!enemy.texture)
		enemy.texture = TextureManager::Get().LoadTexture("Assets/Images/Boss.jpg");
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
	MeshManager::Get().DrawTexturedSquare(
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
		bossEnemy.texture = nullptr;
	}
}

// collision damage (unreferenced param player)
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

	ifs4.open("Assets/Data/GameSave.json");
	if (!ifs4.is_open()) {
		ifs4.clear(); // Clear the fail bit from the first attempt
		ifs4.open("Assets/Data/Config.json");
	}
	rapidjson::IStreamWrapper isw(ifs4);
	bossConfig.ParseStream(isw);

	// Log that loading is complete
	std::cout << "Boss:Load" << std::endl;
	
}

// ----------------------------------------------------------------------------
// Initializes Level 2 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Boss_Initialize()
{
	//bossPlatformMesh = util::CreateSquareMesh();
	
	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise platforms from JSON
	const rapidjson::Value& platforms = bossConfig["level_4"]["platforms"];
	bossPlatforms.clear(); // Clear any existing data

	if (platforms.IsArray()) {
		for (rapidjson::SizeType i = 0; i < platforms.Size(); i++) {
			const rapidjson::Value& platform = platforms[i];

			if (platform.HasMember("x") && platform.HasMember("y") &&
				platform.HasMember("width") && platform.HasMember("height")) {

				Platform newPlatform{};
				newPlatform.x = platform["x"].GetFloat();
				newPlatform.y = platform["y"].GetFloat();
				newPlatform.w = platform["width"].GetFloat();
				newPlatform.h = platform["height"].GetFloat();

				bossPlatforms.push_back(newPlatform);
			}
		}
	}

	// initialise camera
	Camera_Init(globalCam, bossPlayer.pos.x, bossPlayer.pos.y);

	std::cout << "Boss:Initialize" << std::endl;

}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Boss_Update()
{
	std::cout << "Boss:Update" << std::endl;

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
	std::cout << "Boss:Draw" << std::endl;

	// Informing the system about the loop's start
	AESysFrameStart();

	// draw background
	Background_Draw();

	// draw text for level indicator
	LevelIndicator_Draw();

	// draw platforms
	Platforms_Draw(bossPlatforms);

	AESysFrameEnd();
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 2
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Boss_Free()
{
	std::cout << "Boss:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Boss_Unload()
{
	
	bossPlatforms.clear();
	ifs4.close();
	std::cout << "Boss:Unload" << std::endl;

}
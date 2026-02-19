/* Start Header ************************************************************************/
/*!
\file       MainGame.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       February 4 2026
\brief		This file contains the function definitions for the main game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
/*
#include "pch.h"
#include "FileManager.h"
#include "GameStateManager.h"
#include "Utils.h"
#include "Input.h"

#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

#include "Camera.h"
#include "Background.h"
#include "LevelIndicator.h"
#include "Platforms.h"
#include "WinLose.h"

#include "Level1.h"
#include "Level2.h"
#include "Level3.h"

#include "Player.h"
#include "PlayerBullet.h"
#include "PlayerMelee.h"

#include "Boss.h"
#include "enemy.h"//Enemy
#include "EnemyBullet.h"

// --------------------------------------------------GLOBAL VARIABLES START--------------------------------------------------

// ---------------JSON---------------
rapidjson::Document configDoc;
std::ifstream ifs;

// ---------------MESH/TEXTURES---------------

// ---------------GROUND/PLATFORMS/WALLS---------------
float ground;

static std::vector<Platform> platf;
static std::vector<PlatformObstacle> obs;

static std::vector<Platform> wallPlatforms;

// ---------------PLAYER---------------
static Player mainPlayer;

static AEGfxTexture* playerTexture = nullptr;
static AEGfxTexture* playerMeleeTexture = nullptr;
static AEGfxTexture* playerMeleeAttackTexture = nullptr;
static AEGfxTexture* playerMeleeWeaponTexture = nullptr;//for the weapon
static AEGfxTexture* playerGunTexture = nullptr;
static AEGfxTexture* playerGunAttackTexture = nullptr;

// ---------------ENEMY---------------
static Enemy EasyEnemy;
static std::vector<EnemyBullet> enemyBullets;
static Enemy HardEnemy;
static Enemy BossEnemy;

static AEGfxTexture* easyEnemyTexture = nullptr;
static AEGfxTexture* hardEnemyTexture = nullptr;
static AEGfxTexture* hardEnemyAttackTexture = nullptr;
static AEGfxTexture* lowHpOverlayTexture = nullptr;

// ---------------OTHERS---------------
float dt = (float)AEFrameRateControllerGetFrameTime();

// --------------------------------------------------GLOBAL VARIABLES END--------------------------------------------------

void MainGame_Load() {

	// load all levels
	Level1_Load();
	Level2_Load();
	Level3_Load();
	Boss_Load();

}

void MainGame_Initialize() {


	// initialise background
	Background_Initialise();

	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise camera
	ground = -350.0f;
	const float groundHeight = 50.0f;
	float groundTop = ground + groundHeight * 0.5f;

	const float halfScreenHeight = 900 * 0.5f;
	
	float camera_start_y = groundTop + halfScreenHeight;
	Camera_Init(globalCam, mainPlayer.pos.x, camera_start_y);

	// initialise level platforms
	Level1_Initialize();
	Level2_Initialize();
	Level3_Initialize();
	Boss_Initialize();

	// initialise walls from JSON
	const rapidjson::Value& walls = configDoc["level_1"]["walls"];
	wallPlatforms.clear();

	if (walls.IsArray()) {
		for (rapidjson::SizeType i = 0; i < walls.Size(); i++) {
			const rapidjson::Value& wall = walls[i];

			if (wall.HasMember("x") && wall.HasMember("y") &&
				wall.HasMember("width") && wall.HasMember("height")) {

				Platform newWall{};
				newWall.x = wall["x"].GetFloat();
				newWall.y = wall["y"].GetFloat();
				newWall.w = wall["width"].GetFloat();
				newWall.h = wall["height"].GetFloat();

				wallPlatforms.push_back(newWall);
			}
		}
	}

	// initialise obstacles from JSON
	const rapidjson::Value& obstacles = configDoc["level_1"]["obstacles"];
	if (obstacles.IsArray()) {
		for (rapidjson::SizeType i = 0; i < obstacles.Size(); i++) {
			const rapidjson::Value& obstacle = obstacles[i];

			if (obstacle.HasMember("x") && obstacle.HasMember("y") &&
				obstacle.HasMember("width") && obstacle.HasMember("height") && obstacle.HasMember("rotation")) {

				PlatformObstacle newObstacle{};
				newObstacle.x = obstacle["x"].GetFloat();
				newObstacle.y = obstacle["y"].GetFloat();
				newObstacle.w = obstacle["width"].GetFloat();
				newObstacle.h = obstacle["height"].GetFloat();
				newObstacle.r = obstacle["rotation"].GetFloat();
				obs.push_back(newObstacle);
			}
		}
	}

	// --------------------------------------------------PLAYER INITIALISE START--------------------------------------------------

	// initialise player
	float playerX = configDoc["level_1"]["player"]["x"].GetFloat();
	float playerY = configDoc["level_1"]["player"]["y"].GetFloat();
	Player_Init(mainPlayer, playerX, playerY);
	mainPlayer.grounded = 1;

	// load player resources
	
	if (!playerTexture)
		playerTexture = AEGfxTextureLoad("Assets/Images/player.jpg");

	if (!playerMeleeTexture)
		playerMeleeTexture = AEGfxTextureLoad("Assets/Images/PlayerMelee.jpg");

	if (!playerMeleeWeaponTexture)
	{
		playerMeleeWeaponTexture = AEGfxTextureLoad("Assets/Images/PlayerMeleeWeapon.png");
	}

	if (!playerMeleeAttackTexture)
		playerMeleeAttackTexture =
		AEGfxTextureLoad("Assets/Images/PlayerMeleeAttack.jpg");

	if (!playerGunTexture)
		playerGunTexture = AEGfxTextureLoad("Assets/Images/PlayerGun.jpg");

	if (!playerGunAttackTexture)
		playerGunAttackTexture = AEGfxTextureLoad("Assets/Images/PlayerGunAttack.jpg");

	// assign graphic resources to player
	mainPlayer.texture = playerTexture;

	mainPlayer.meleeTexture = playerMeleeTexture;
	mainPlayer.meleeAttackTexture = playerMeleeAttackTexture;
	mainPlayer.meleeWeaponTexture = playerMeleeWeaponTexture;

	mainPlayer.gunTexture = playerGunTexture;
	mainPlayer.gunAttackTexture = playerGunAttackTexture;

	// bind player to input system
	Input_SetPlayer(&mainPlayer);

	// --------------------------------------------------PLAYER INITIALISE END--------------------------------------------------

	// --------------------------------------------------ENEMY INITIALISE START--------------------------------------------------

	// initialise enemy from JSON
	const rapidjson::Value& enemies = configDoc["level_1"]["enemies"];
	if (enemies.IsArray() && enemies.Size() > 0) {
		float enemyX = enemies[0]["x"].GetFloat();
		float enemyY = enemies[0]["y"].GetFloat();
		Enemy_Init(EasyEnemy, enemyX, enemyY);

		EasyEnemy.mesh = enemyMesh;
		EasyEnemy.texture = easyEnemyTexture;
		EasyEnemy.normalTexture = easyEnemyTexture;

		float hardEnemyX = enemies[1]["x"].GetFloat(); //JSON index 1 for HardEnemy
		float hardEnemyY = enemies[1]["y"].GetFloat();
		HardEnemy_Init(HardEnemy, hardEnemyX, hardEnemyY);

		HardEnemy.texture = hardEnemyTexture;
		HardEnemy.normalTexture = hardEnemyTexture;
		HardEnemy.attackTexture = hardEnemyAttackTexture;

		// boss
		//mr pogba
		float bossX = configDoc["level_4"]["enemies"][0]["x"].GetFloat();
		float bossY = configDoc["level_4"]["enemies"][0]["y"].GetFloat();
		BossEnemy_Init(BossEnemy, bossX, bossY);
	}

	// load enemy resources

	if (!easyEnemyTexture)
		easyEnemyTexture = AEGfxTextureLoad("Assets/Images/easyenemy.jpg");

	if (!hardEnemyTexture)
		hardEnemyTexture = AEGfxTextureLoad("Assets/Images/hardenemy.jpg");

	if (!hardEnemyAttackTexture)
		hardEnemyAttackTexture = AEGfxTextureLoad("Assets/Images/hardenemy_attack.jpg");

	//for enemy lowhp
	if (!lowHpOverlayTexture)
	{
		lowHpOverlayTexture = AEGfxTextureLoad("Assets/Images/LowHpOverlay.jpg");
	}

	// for enemy low HP overlay
	if (!lowHpOverlayTexture)
	{
		lowHpOverlayTexture = AEGfxTextureLoad("Assets/Images/LowHpOverlay.jpg");
	}

	// --------------------------------------------------ENEMY INITIALISE END--------------------------------------------------

}

void MainGame_Update() {

	// --------------------------------------------------BACKGROUND UPDATE START--------------------------------------------------

	// update background based on player's y position/debug
	float backgroundY = globalCam.debugCam ? globalCam.y : mainPlayer.pos.y;
	Background_Update(backgroundY);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	// update when section changes
	LevelIndicator_Update(dt);

	// --------------------------------------------------BACKGROUND UPDATE END--------------------------------------------------

	// --------------------------------------------------PLATFORMS UPDATE START--------------------------------------------------



	// --------------------------------------------------PLATFORMS UPDATE END--------------------------------------------------

	// --------------------------------------------------PLAYER UPDATE START--------------------------------------------------
	


	// --------------------------------------------------PLAYER UPDATE END--------------------------------------------------

	// --------------------------------------------------ENEMY UPDATE START--------------------------------------------------

	

	// --------------------------------------------------ENEMY UPDATE END--------------------------------------------------

	// --------------------------------------------------DEBUG CODES START--------------------------------------------------

	// toggle use debug cam
	if (AEInputCheckTriggered(AEVK_0)) {

		globalCam.debugCam = !globalCam.debugCam;

	}

	if (globalCam.debugCam) {

		Camera_Debug(globalCam, dt);

	}
	else {

		// camera follows player
		Camera_FollowPlayer(globalCam, mainPlayer.pos.x, mainPlayer.pos.y, dt);

		// apply camera
		Camera_Apply(globalCam);

	}

	// DEBUG: teleport to boss platform
	// we can't be manualy climbing everytiem guys this is painful </3
	if (AEInputCheckTriggered(AEVK_6))
	{
		mainPlayer.pos.x = 0.0f;
		mainPlayer.pos.y = 1900.0f;
		mainPlayer.vel.x = 0.0f;
		mainPlayer.vel.y = 0.0f;
	}

	// DEBUG: back to bottom platform
	if (AEInputCheckTriggered(AEVK_7))
	{
		mainPlayer.pos.x = 0.0f;
		mainPlayer.pos.y = -300.0f;
		mainPlayer.vel.x = 0.0f;
		mainPlayer.vel.y = 0.0f;
	}

	// --------------------------------------------------DEBUG CODES END--------------------------------------------------

}

void MainGame_Draw() {

	// inform system about loop's start
	AESysFrameStart();

	AESysFrameEnd();

}

void MainGame_Free() {

	// free player
	Player_Free(mainPlayer);

	EnemyBullet_Free();

}

void MainGame_Unload() {

	// unload textures
	AEGfxTextureUnload(playerTexture);
	AEGfxTextureUnload(playerMeleeTexture);
	AEGfxTextureUnload(playerMeleeAttackTexture);
	AEGfxTextureUnload(playerMeleeWeaponTexture);
	AEGfxTextureUnload(playerGunTexture);
	AEGfxTextureUnload(playerGunAttackTexture);

	playerTexture = nullptr;
	playerMeleeTexture = nullptr;
	playerMeleeAttackTexture = nullptr;
	playerMeleeWeaponTexture = nullptr;
	playerGunTexture = nullptr;
	playerGunAttackTexture = nullptr;

	// close file
	ifs.close();

}
*/
/* Start Header ************************************************************************/
/*!
\file Level1.cpp
\author Joash ng, joash.ng, 2502780
		Sim Hui Min, s.huimin, 2503506
\par joash.ng@digipen.edu
	 s.huimin@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Level 1 of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "FileManager.h"
#include "GameStateManager.h"
#include "Level1.h"
#include "Player.h"
#include "Utils.h"
#include "Input.h"
#include "enemy.h"//Enemy
#include "Background.h"
#include "LevelIndicator.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>
#include "Platforms.h"
#include "EnemyBullet.h"
#include "PlayerBullet.h"
#include <iostream>
#include "Camera.h"
#include "WinLose.h"
#include "Boss.h"

static Player lv1Player;
static Enemy EasyEnemy; //Enemy
static std::vector<EnemyBullet> enemyBullets;//Enemy
static Enemy HardEnemy;
rapidjson::Document level1Config;

static Enemy BossEnemy;  // boss - temporary for demo as everything's in this file..
static std::vector<PlayerBullet> playerBullets; // player gun pool // player bullets

static int previousSelection = -1;
const float LEVEL2_START_Y = sectionHeight[0];

AEGfxVertexList* lv1mesh;
AEGfxVertexList* triangleMesh;
std::ifstream ifs;
float ground;

/*
// platforms array
static std::vector<Platform> level1Platforms = {
	{    0.0f, -200.0f, 500.0f, 40.0f },
	{ -450.0f,  -50.0f, 250.0f, 40.0f },
	{  100.0f,	 85.0f, 450.0f, 40.0f },
	{  600.0f,	190.0f, 300.0f, 40.0f },
	{  175.0f,	300.0f, 250.0f, 40.0f },
	{ -300.0f,	450.0f, 520.0f, 40.0f }
};
*/
// platforms array - will be loaded from JSON
static std::vector<Platform> level1Platforms;
static std::vector<Platform> wallPlatforms;
static std::vector<PlatformObstacle> level1Obstacles;
static std::vector<Platform> level2Platforms = {
	{  255.0f,  610.0f, 400.0f, 40.0f, true},
	{ -350.0f,  700.0f, 300.0f, 40.0f, true },
	{  650.0f,  700.0f, 200.0f, 40.0f, true },
	{  190.0f,  840.0f, 500.0f, 40.0f, false }, // toggled by button
	{ -240.0f,  950.0f, 200.0f, 40.0f, false}, // toggled by button
	{  500.0f,  999.0f, 300.0f, 40.0f, true }
};
static std::vector<PlatformButton> level2Buttons = {
	{ 255.0f, 640.0f, 60.0f, 20.0f, 3 },   // on platform 0, toggles platform index 3
	{ -350.0f, 730.0f, 60.0f, 20.0f, 4 },   // on platform 1 (y=700, top=720, button center=730)
};
static std::vector<Platform> level3Platforms = {
	{    0.0f,  1100.0f, 200.0f, 40.0f },
	{  300.0f,  1250.0f, 325.0f, 40.0f },
	{ -300.0f,  1250.0f, 325.0f, 40.0f },
	{    0.0f,  1390.5f, 150.0f, 40.0f },
	{  300.0f,  1499.9f, 250.0f, 40.0f },
	{ -300.0f,  1499.9f, 250.0f, 40.0f }
};
static std::vector<Platform> bossPlatforms = {
	{  600.0f,  1650.0f, 250.0f, 40.0f },
	{ -600.0f,  1650.0f, 250.0f, 40.0f },
	{    0.0f,  1800.0f, 1000.0f, 40.0f }
};

//static const int platformCount = sizeof(level1Platforms) / sizeof(level1Platforms[0]);

// ----------------------------------------------------------------------------
// Loads Level 1 resources and initial data
// Reads the Level1_Counter value from a text file to determine level duration
// ----------------------------------------------------------------------------
void Level1_Load()
{
    
	ifs.open("Assets/Data/GameSave.json");
	if (ifs.is_open()) {
		rapidjson::IStreamWrapper isw(ifs);
		level1Config.ParseStream(isw);
	}
	else if (!ifs.is_open()) {
		ifs.clear(); // Clear the fail bit from the first attempt
		ifs.open("Assets/Data/GameConfig.json");
		rapidjson::IStreamWrapper isw(ifs);
		level1Config.ParseStream(isw);
	}
    std::cout << "Level1:Load" << std::endl;
}

// ----------------------------------------------------------------------------
// Initializes Level 1 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level1_Initialize()
{
	//initialise meshes
	lv1mesh = util::CreateSquareMesh();
	triangleMesh = util::CreateTriangleMesh();
	// initialise background
	Background_Initialise();

	// initialise level indicator
	LevelIndicator_Initialize();

	// Player Initialization
	ground = -350.0f;
	const float groundHeight = 50.0f;
	float groundTop = ground + groundHeight * 0.5f;

	float playerX = level1Config["level_1"]["player"]["x"].GetFloat();
	float playerY = level1Config["level_1"]["player"]["y"].GetFloat();
	Player_Init(lv1Player, playerX, playerY);
	lv1Player.grounded = 1;

	// Bind the level player to the input system
	Input_SetPlayer(&lv1Player);

	// Enemy Initialization from JSON
	const rapidjson::Value& enemies = level1Config["level_1"]["enemies"];
	if (enemies.IsArray() && enemies.Size() > 0) {
		float enemyX = enemies[0]["x"].GetFloat();
		float enemyY = enemies[0]["y"].GetFloat();
		Enemy_Init(EasyEnemy, enemyX, enemyY);

		float hardEnemyX = enemies[1]["x"].GetFloat(); //JSON index 1 for HardEnemy
		float hardEnemyY = enemies[1]["y"].GetFloat();
		HardEnemy_Init(HardEnemy, hardEnemyX, hardEnemyY);

		// boss
		float bossX = level1Config["level_4"]["enemies"][0]["x"].GetFloat();
		float bossY = level1Config["level_4"]["enemies"][0]["y"].GetFloat();
		BossEnemy_Init(BossEnemy, bossX, bossY);
	}

	// Platform Initialization from JSON
	const rapidjson::Value& platforms = level1Config["level_1"]["platforms"];
	level1Platforms.clear(); // Clear any existing data

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

				level1Platforms.push_back(newPlatform);
			}
		}
	}

	// Wall Initialization from JSON
	const rapidjson::Value& walls = level1Config["level_1"]["walls"];
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

	//initialise obstacles
	const rapidjson::Value& obstacles = level1Config["level_1"]["obstacles"];
	if (obstacles.IsArray()) {
		for (rapidjson::SizeType i = 0; i < obstacles.Size(); i++) {
			const rapidjson::Value& obstacle = obstacles[i];

			if (obstacle.HasMember("x") && obstacle.HasMember("y") &&
				obstacle.HasMember("width") && obstacle.HasMember("height") && obstacle.HasMember("rotation") ){

				PlatformObstacle newObstacle{};
				newObstacle.x = obstacle["x"].GetFloat();
				newObstacle.y = obstacle["y"].GetFloat();
				newObstacle.w = obstacle["width"].GetFloat();
				newObstacle.h = obstacle["height"].GetFloat();
				newObstacle.r = obstacle["rotation"].GetFloat();
				level1Obstacles.push_back(newObstacle);
			}
		}
	}

	// initialise camera
	const float halfScreenHeight = 900 * 0.5f;
	float camera_start_y = groundTop + halfScreenHeight;
	Camera_Init(globalCam, lv1Player.pos.x, camera_start_y);

	previousSelection = -1;

	std::cout << "Level1:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates Level 1 logic every frame
// Decrements the counter and checks for level completion
// ----------------------------------------------------------------------------
void Level1_Update()
{	
	// Player Update
	static float playerPrevY = 0.0f;
	playerPrevY = lv1Player.pos.y;

	float dt = (float)AEFrameRateControllerGetFrameTime();
	Player_Update(lv1Player, dt);

	Player_CheckBulletCollisions(EasyEnemy);
	Player_CheckBulletCollisions(HardEnemy);
	Player_CheckBulletCollisions(BossEnemy);
	
	// boss killed -> win
	if (!BossEnemy.isAlive)
	{
		textScreenMessage = "You Win";
		next = GS_WINLOSE;
	}

	// fall below camera -> lose
	const float halfScreenHeight = 900 * 0.5f;
	float camBottomY = globalCam.y - halfScreenHeight;
	float playerTopY = lv1Player.pos.y + lv1Player.height * 0.5f;

	if (playerTopY < camBottomY - 50.0f)
	{
		textScreenMessage = "You Lose";
		next = GS_WINLOSE;
	}

	lv1Player.grounded = 0;

	const float groundHeight = 50.0f;
	float groundTop = ground + groundHeight * 0.5f;

	if (lv1Player.pos.y - lv1Player.height * 0.5f <= groundTop)
	{
		lv1Player.pos.y = groundTop + lv1Player.height * 0.5f;
		lv1Player.vel.y = 0.0f;
		lv1Player.grounded = 1;
		playerPrevY = lv1Player.pos.y;
	}

	// Player collision
	if (lv1Player.vel.y <= 0.0f)
	{
		float playerPrevBottom = playerPrevY - lv1Player.height * 0.5f;
		float playerCurrBottom = lv1Player.pos.y - lv1Player.height * 0.5f;
		
		auto CheckPlatformLanding = [&](const std::vector<Platform>& platforms)->bool
			{
				for (const Platform& pf : platforms)
				{
					if (!pf.active) continue;  //

					float pfLeft = pf.x - pf.w * 0.5f;
					float pfRight = pf.x + pf.w * 0.5f;
					float pfTop = pf.y + pf.h * 0.5f;

					float playerLeft = lv1Player.pos.x - lv1Player.width * 0.5f;
					float playerRight = lv1Player.pos.x + lv1Player.width * 0.5f;

					bool overlapX = (playerRight >= pfLeft) && (playerLeft <= pfRight);
					bool landedThisFrame = (playerPrevBottom >= pfTop) && (playerCurrBottom <= pfTop);

					if (overlapX && landedThisFrame)
					{
						lv1Player.pos.y = pfTop + lv1Player.height * 0.5f;
						lv1Player.vel.y = 0.0f;
						lv1Player.grounded = 1;

						playerPrevY = lv1Player.pos.y;
						return true;
					}
				}
				return false;
			};

		if (!CheckPlatformLanding(level1Platforms) &&
			!CheckPlatformLanding(level2Platforms) &&
			!CheckPlatformLanding(level3Platforms))
		{
			CheckPlatformLanding(bossPlatforms);
		}

		if (CheckObstacleCollision(lv1Player, level1Obstacles[0])) //hit spikes
		{
			textScreenMessage = "You Lose"; //change when hp system implemented
			next = GS_WINLOSE;
		}
	}

	// Wall collision
	for (const Platform& w : wallPlatforms)
	{
		if (!w.active) continue;

		float wLeft = w.x - w.w * 0.5f;
		float wRight = w.x + w.w * 0.5f;
		float wBottom = w.y - w.h * 0.5f;
		float wTop = w.y + w.h * 0.5f;

		float playerLeft = lv1Player.pos.x - lv1Player.width * 0.5f;
		float playerRight = lv1Player.pos.x + lv1Player.width * 0.5f;
		float playerBottom = lv1Player.pos.y - lv1Player.height * 0.5f;
		float playerTop = lv1Player.pos.y + lv1Player.height * 0.5f;

		bool overlapX = (playerRight > wLeft) && (playerLeft < wRight);
		bool overlapY = (playerTop > wBottom) && (playerBottom < wTop);

		if (!(overlapX && overlapY))
		{
			continue;
		}

		float pushRight = wRight - playerLeft;
		float pushLeft = playerRight - wLeft;

		if (pushRight < pushLeft)
		{
			lv1Player.pos.x += pushRight;
		}
		else
		{
			lv1Player.pos.x -= pushLeft;
		}
	}

	// level 2 button toggle
	for (auto& btn : level2Buttons)
	{
		float btnLeft = btn.x - btn.w * 0.5f;
		float btnRight = btn.x + btn.w * 0.5f;
		float btnTop = btn.y + btn.h * 0.5f;
		float btnBottom = btn.y - btn.h * 0.5f;

		float playerLeft = lv1Player.pos.x - lv1Player.width * 0.5f;
		float playerRight = lv1Player.pos.x + lv1Player.width * 0.5f;
		float playerBottom = lv1Player.pos.y - lv1Player.height * 0.5f;

		bool overlapX = (playerRight >= btnLeft) && (playerLeft <= btnRight);
		bool landedOnButton = (playerBottom <= btnTop) && (playerBottom >= btnBottom);
		bool isPressed = overlapX && landedOnButton && lv1Player.grounded;

		// only toggle on the frame it becomes pressed
		if (isPressed && !btn.wasPressed)
		{
			level2Platforms[btn.platformIndex].active = !level2Platforms[btn.platformIndex].active;
		}

		btn.wasPressed = isPressed;
	}


	//player melee vs EasyEnemy hashtag evil
	if (lv1Player.isAttacking && EasyEnemy.isAlive)
	{
		float playerHalfW = lv1Player.width * 0.5f;
		float playerHalfH = lv1Player.height * 0.5f;
		float enemyHalfW = EasyEnemy.width * 0.5f;
		float enemyHalfH = EasyEnemy.height * 0.5f;

		bool overlapX = fabs(lv1Player.pos.x - EasyEnemy.pos.x) < (playerHalfW + enemyHalfW);
		bool overlapY = fabs(lv1Player.pos.y - EasyEnemy.pos.y) < (playerHalfH + enemyHalfH);

		if (overlapX && overlapY)
		{
			Enemy_OnHit(EasyEnemy);
		}
	}


	//Player melee vs HardEnemy
	if (lv1Player.isAttacking && HardEnemy.isAlive)
	{
		float playerHalfW = lv1Player.width * 0.5f;
		float playerHalfH = lv1Player.height * 0.5f;
		float enemyHalfW = HardEnemy.width * 0.5f;
		float enemyHalfH = HardEnemy.height * 0.5f;

		bool overlapX = fabs(lv1Player.pos.x - HardEnemy.pos.x) < (playerHalfW + enemyHalfW);
		bool overlapY = fabs(lv1Player.pos.y - HardEnemy.pos.y) < (playerHalfH + enemyHalfH);

		if (overlapX && overlapY)
		{
			Enemy_OnHit(HardEnemy);
		}
	}


	//enemy update
	Enemy_Update(EasyEnemy, dt);//Enemy
	HardEnemy_Update(HardEnemy, dt);
	BossEnemy_Update(BossEnemy, dt);

	// Enemy shooting
	if (EasyEnemy.isAlive)
	{
		//Count down the shooting timer
		EasyEnemy.shootTimer -= dt;

		if (EasyEnemy.shootTimer <= 0.0f)
		{
			//Spawn a bullet
			EnemyBullet bullet{};
			bullet.pos = EasyEnemy.pos;

			//line up bullet Y with player(test)
			//bullet.pos.y = lv1Player.pos.y; 

			bullet.startPos = bullet.pos;
			bullet.direction = EasyEnemy.direction;

			bullet.speed = level1Config["level_1"]["enemies"][0]["bullet_speed"].GetFloat();
			if (bullet.speed <= 0.0f) bullet.speed = 50.0f;

			bullet.damage = level1Config["level_1"]["enemies"][0]["bullet_damage"].GetFloat();
			bullet.maxRange = 1600.0f;
			bullet.active = true;

			enemyBullets.push_back(bullet);


			//reset shoot timer only when shooting
			EasyEnemy.shootTimer = EasyEnemy.shootCooldown;
		}
	}

	//update bullets
	for (auto& bullet : enemyBullets)
	{
		if (!bullet.active) continue;

		bullet.pos.x += bullet.direction * bullet.speed * dt;

		float traveled = fabs(bullet.pos.x - bullet.startPos.x);
		if (traveled >= bullet.maxRange)
			bullet.active = false;
	}

	// bullet collisions
	for (auto& bullet : enemyBullets)
	{
		if (!bullet.active) continue;

		//bullet and player size
		float bulletHalfW = 20.0f;
		float bulletHalfH = 20.0f;
		float playerHalfW = lv1Player.width * 0.5f;
		float playerHalfH = lv1Player.height * 0.5f;

		bool overlapX = fabs(bullet.pos.x - lv1Player.pos.x) < (bulletHalfW + playerHalfW);
		bool overlapY = fabs(bullet.pos.y - lv1Player.pos.y) < (bulletHalfH + playerHalfH);

		if (overlapX && overlapY)
		{
			Player_ApplyDamage(lv1Player, bullet.damage);
			bullet.active = false;
			/*
			//debug print to verify collision
			std::cout << "Player hit! HP: " << lv1Player.hp
				<< " Bullet pos: (" << bullet.pos.x << "," << bullet.pos.y << ") "
				<< " Player pos: (" << lv1Player.pos.x << "," << lv1Player.pos.y << ")"
				<< std::endl;
				*/
		}
	}

	//Hard enemy
	// HardEnemy collision with player
	float playerHalfW = lv1Player.width * 0.5f;
	float playerHalfH = lv1Player.height * 0.5f;
	float enemyHalfW = HardEnemy.width * 0.5f;
	float enemyHalfH = HardEnemy.height * 0.5f;

	bool overlapX = fabs(HardEnemy.pos.x - lv1Player.pos.x) < (enemyHalfW + playerHalfW);
	bool overlapY = fabs(HardEnemy.pos.y - lv1Player.pos.y) < (enemyHalfH + playerHalfH);

	if (overlapX && overlapY)
	{
		HardEnemy_OnCollision(HardEnemy, lv1Player);
	}

	// Boss enemy
	// Boss collision with player
	float bossHalfW = BossEnemy.width * 0.5f;
	float bossHalfH = BossEnemy.height * 0.5f;
	float playerHalfW_b = lv1Player.width * 0.5f;
	float playerHalfH_b = lv1Player.height * 0.5f;

	bool bossOverlapX = fabs(BossEnemy.pos.x - lv1Player.pos.x) < (bossHalfW + playerHalfW_b);
	bool bossOverlapY = fabs(BossEnemy.pos.y - lv1Player.pos.y) < (bossHalfH + playerHalfH_b);

	if (bossOverlapX && bossOverlapY)
	{
		BossEnemy_OnCollision(BossEnemy, lv1Player);
	}

	// toggle use debug cam
	if (AEInputCheckTriggered(AEVK_1)) {

		globalCam.debugCam = !globalCam.debugCam;

	}

	if (globalCam.debugCam) {

		Camera_Debug(globalCam, dt);

	}
	else {

		// camera follows player
		Camera_FollowPlayer(globalCam, lv1Player.pos.x, lv1Player.pos.y, dt);

		// apply camera
		Camera_Apply(globalCam);

	}

	// update background based on y axis
	Background_Update(globalCam.y);

	// check for section change
	int currentSection = Background_CurrentSection();

	if (currentSection != previousSection) {

		LevelIndicator_Show(currentSection);
		previousSection = currentSection;

	}

	//// exit level 1 & goes to level 2
	//const float endOfLevel1 = sectionHeight[0];

	//if (globalCam.y >= endOfLevel1) {

	//	next = GS_LEVEL2;

	//}

	// update when section changes
	LevelIndicator_Update(dt);

	std::cout << "Level1:Update" << std::endl;

	// DEBUG: teleport to boss platform
	// we can't be manualy climbing everytiem guys this is painful </3
	if (AEInputCheckTriggered(AEVK_6))
	{
		lv1Player.pos.x = 0.0f;
		lv1Player.pos.y = 1900.0f;
		lv1Player.vel.x = 0.0f;
		lv1Player.vel.y = 0.0f;
	}

	// DEBUG: back to bottom platform
	if (AEInputCheckTriggered(AEVK_7))
	{
		lv1Player.pos.x = 0.0f;
		lv1Player.pos.y = -300.0f;
		lv1Player.vel.x = 0.0f;
		lv1Player.vel.y = 0.0f;
	}
	
}

// ----------------------------------------------------------------------------
// Renders Level 1 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Level1_Draw()
{
	// Informing the system about the loop's start
	AESysFrameStart();

	// draw background
	Background_Draw();

	// draw platforms
	Platforms_Draw(lv1mesh, level1Platforms);
	Platforms_Draw(lv1mesh, level2Platforms);
	Platforms_Draw(lv1mesh, level3Platforms);
	Platforms_Draw(lv1mesh, bossPlatforms);
	PlatformButton_Draw(lv1mesh, level2Buttons, level2Platforms);
	PlatformsObstacle_Draw(triangleMesh, level1Obstacles);
	Platforms_Draw(lv1mesh, wallPlatforms);

	util::DrawSquare(lv1mesh, 0.0f, ground, 1600.0f, 50.0f, 0, 0, 0); // Draw Ground (Texture TBA?)
	Player_Draw(lv1Player);

	Enemy_Draw(EasyEnemy);//Enemy
	for (const auto& bullet : enemyBullets)
	{
		if (!bullet.active) continue;

		EnemyBullet_Draw(bullet);
	}

	Enemy_Draw(HardEnemy);
	BossEnemy_Draw(BossEnemy);

	// player bullets
	for (const auto& b : playerBullets)
	{
		PlayerBullet_Draw(b);
	}


	// draw text for level indicator
	LevelIndicator_Draw();

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
	EnemyBullet_Free();
	std::cout << "Level1:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 1 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level1_Unload()
{
	AEGfxMeshFree(lv1mesh);
	AEGfxMeshFree(triangleMesh);
	ifs.close();
	std::cout << "Level1:Unload" << std::endl;
}
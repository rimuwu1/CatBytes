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
#include "FileManager.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>

static Player lv3Player;

AEGfxVertexList* lv3mesh;

rapidjson::Document level3Config;

std::ifstream ifs3;

/* platforms array
static std::vector<Platform> level3Platforms = {
	{    0.0f,  1100.0f, 200.0f, 40.0f },
	{  300.0f,  1250.0f, 325.0f, 40.0f },
	{ -300.0f,  1250.0f, 325.0f, 40.0f },
	{    0.0f,  1390.5f, 150.0f, 40.0f },
	{  300.0f,  1499.9f, 250.0f, 40.0f },
	{ -300.0f,  1499.9f, 250.0f, 40.0f }
};
*/

// platforms array - loaded from JSON
std::vector<Platform> level3Platforms;

// ----------------------------------------------------------------------------
// Loads Level 3 resources and initial data
// Reads the initial number of lives from a text file
// ----------------------------------------------------------------------------
void Level3_Load()
{
	
	ifs3.open("Assets/Data/GameSave.json");
	if (!ifs3.is_open()) {
		ifs3.clear(); // Clear the fail bit from the first attempt
		ifs3.open("Assets/Data/Config.json");
	}
	rapidjson::IStreamWrapper isw(ifs3);
	level3Config.ParseStream(isw);

	std::cout << "Level3:Load" << std::endl;

}

// ----------------------------------------------------------------------------
// Initializes Level 3 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level3_Initialize()
{

	lv3mesh = util::CreateSquareMesh();
	
	// initialise level indicator
	LevelIndicator_Initialize();

	// initialise platforms from JSON
	const rapidjson::Value& platforms = level3Config["level_3"]["platforms"];
	level3Platforms.clear(); // Clear any existing data

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

				level3Platforms.push_back(newPlatform);
			}
		}
	}

	// initialise camera
	Camera_Init(globalCam, lv3Player.pos.x, lv3Player.pos.y);

	std::cout << "Level3:Initialize" << std::endl;

}

// ----------------------------------------------------------------------------
// Updates Level 3 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Level3_Update()
{
	std::cout << "Level3:Update" << std::endl;

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

		return;

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

	std::cout << "Level3:Draw" << std::endl;
	
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
	std::cout << "Level3:Free" << std::endl;
	
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level3_Unload()
{
	AEGfxMeshFree(lv3mesh);
	level3Platforms.clear();
	ifs3.close();
	std::cout << "Level3:Unload" << std::endl;

}
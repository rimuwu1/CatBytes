/* Start Header ************************************************************************/
/*!
\file       Platforms.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
			Joash Ng, joash.ng, 2502780
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
			joash.ng@digipen.edu
\date       January 26 2026
\brief		This file contains the structs & function declarations needed for the platforms.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include <vector>
#include "Player.h"

struct Platform {
	float x, y, w, h;
	mutable bool active;				// allow modification even if it's const
};


struct PlatformButton {
	float x, y, w, h;
	int platformIndex;					// remove after adding asset
	std::vector<int> platformIndices;	// use vector to allow multiple platforms toggle
	mutable bool wasPressed;			// allow modification even if it's const
	mutable bool prevState;				// track switch's previous state (active/inactive)
	mutable bool spriteInitialized;		// track sprite initialization
};

struct PlatformObstacle {
	float x, y, w, h, r;
	bool isSpike;
	//bool active = true; //can toggle off if bullet collision for example
};

struct Checkpoint {
	float x, y, w, h, r;
};

void PlatformButton_Draw(const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms, const Player& player);

void Platforms_Draw(const std::vector<Platform>& platforms, AEGfxTexture* leftTex, AEGfxTexture* midTex, AEGfxTexture* rightTex);

bool Platform_CollisionCheck(Player& player, float& previosY, const std::vector<Platform>& platforms);

void PlatformsObstacle_Draw(const std::vector<PlatformObstacle>& obstacles);

bool CheckObstacleCollision(const Player& player, const std::vector<PlatformObstacle>& obstacle);

void Platforms_OffsetY(std::vector<Platform>& platforms, float offsetY);

void WallCollisionCheck(Player& player, const std::vector<Platform>& wallPlatforms);

void CheckpointDraw(const std::vector<Checkpoint>& checkpoint, const Player& player);

void CheckpointCollisionCheck(const Player& player, const std::vector<Checkpoint>& checkpoint, bool& checkpointHit, bool& checkpointInRange);


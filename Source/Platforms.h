/* Start Header ************************************************************************/
/*!
\file       Platforms.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
\date       January 26 2026
\brief

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
	bool active = true;
};

// global platform mesh
//extern AEGfxVertexList* platformMesh;

//void Platforms_Initialize();

struct PlatformButton {
	float x, y, w, h;
	int platformIndex;  // which index in the platform array this toggles
	bool wasPressed = false;
};

struct PlatformObstacle {
	float x, y, w, h, r;
	//bool active = true; //can toggle off if bullet collision for example
};
void PlatformButton_Draw(AEGfxVertexList* mesh, const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms);

void Platforms_Draw(AEGfxVertexList* mesh, const std::vector<Platform> &platform);

bool Platform_CollisionCheck(Player& player, float& previosY, const std::vector<Platform>& platforms);

void PlatformsObstacle_Draw(AEGfxVertexList* mesh, const std::vector<PlatformObstacle>& obstacles);

bool CheckObstacleCollision(const Player& player, const PlatformObstacle& obstacle);

void Platforms_OffsetY(std::vector<Platform>& platforms, float offsetY);
/* Start Header ************************************************************************/
/*!
\file       Platforms.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 26 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Platforms.h"
#include "Utils.h"

AEGfxVertexList* platformMesh = nullptr;

void Platforms_Initialize() {

	// initialise platform mesh
	platformMesh = util::CreateSquareMesh();

}

void Platforms_Draw(const std::vector<Platform>& platform) {

	// for each Platform named pf in container platform
	for (const Platform& pf : platform) {

		util::DrawSquare(platformMesh, pf.x, pf.y, pf.w, pf.h, 60, 60, 60);

	}

}

float Get_Highest_Platform_YPos(const std::vector<Platform>& platforms) {

	float highestPlatform = platforms[0].y + platforms[0].h / 2;

	for (const auto& pf : platforms) {

		float platformTop = pf.y + pf.h / 2;

		if (platformTop > highestPlatform) {

			highestPlatform = platformTop;

		}

	}

	return highestPlatform;

}
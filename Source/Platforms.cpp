/* Start Header ************************************************************************/
/*!
\file       Platforms.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
\date       January 26 2026
\brief		This file implements the functions for the game's platforms.

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

//AEGfxVertexList* platformMesh = nullptr;

//void Platforms_Initialize() {

	// initialise platform mesh
	//platformMesh = util::CreateSquareMesh();

//}

void Platforms_Draw(AEGfxVertexList* mesh, const std::vector<Platform>& platform) {

	// for each Platform named pf in container platform
	/*for (const Platform& pf : platform) {

		util::DrawSquare(mesh, pf.x, pf.y, pf.w, pf.h, 255, 178, 102);

	}*/

	for (const Platform& pf : platform) {
		if (!pf.active) continue;  // skip inactive
		util::DrawSquare(mesh, pf.x, pf.y, pf.w, pf.h, 255, 178, 102);
	}

}

void PlatformButton_Draw(AEGfxVertexList* mesh, const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms)
{
	for (int i = 0; i < (int)buttons.size(); i++)
	{
		const PlatformButton& btn = buttons[i];
		// green if linked platform is active, red if not
		if (platforms[btn.platformIndex].active)
			util::DrawSquare(mesh, btn.x, btn.y, btn.w, btn.h, 0, 255, 0);
		else
			util::DrawSquare(mesh, btn.x, btn.y, btn.w, btn.h, 255, 0, 0);
	}
}

void Platforms_OffsetY(std::vector<Platform>& platforms, float offsetY)
{
	for (Platform& pf : platforms)
	{
		pf.y += offsetY;
	}
}
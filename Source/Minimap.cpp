/* Start Header ************************************************************************/
/*!
\file	Minimap.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par	kerwinajijie.wong@digipen.edu
\date	February, 19, 2026
\brief	This file contains the function definitions for rendering the in-game minimap
		that maps player world position proportionally within defined world bounds.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "Minimap.h"
#include "MeshManager.h"

static float ClampMinimap(float t)
{
	if (t < 0.0f)
	{
		return 0.0f;
	}
	if (t > 1.0f)
	{
		return 1.0f;
	}
	return t;
}

void Minimap_Draw(const Minimap& mm, float playerWorldX, float playerWorldY)
{
	if (!mm.active)
	{
		return;
	}

	// Minimap panel
	MeshManager::Get().DrawSquare(mm.x, mm.y, mm.w, mm.h, 70, 70, 90);

	// Range for normalising between world & minimap
	float rangeX = (mm.worldMaxX - mm.worldMinX);
	if (rangeX <= 0.0001f)
	{
		rangeX = 1.0f;
	}

	float rangeY = (mm.worldMaxY - mm.worldMinY);
	if (rangeY <= 0.0001f)
	{
		rangeY = 1.0f;
	}

	// Normalise player position
	float tx = (playerWorldX - mm.worldMinX) / rangeX;
	tx = ClampMinimap(tx);

	float ty = (playerWorldY - mm.worldMinY) / rangeY;
	ty = ClampMinimap(ty);

	float panelLeft = mm.x - mm.w * 0.5f;
	float panelBottom = mm.y - mm.h * 0.5f;

	float dotX = panelLeft + (tx * mm.w);
	float dotY = panelBottom + (ty * mm.h);

	MeshManager::Get().DrawSquare(dotX, dotY, mm.dotSize, mm.dotSize, 255, 0, 0);
}
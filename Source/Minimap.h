/* Start Header ************************************************************************/
/*!
\file	Minimap.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par	kerwinajijie.wong@digipen.edu
\date	February, 19, 2026
\brief	This file contains the function declarations for ...

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"

struct Minimap {
	float x, y, w, h, dotSize;
	bool active = true;
	float worldMinX, worldMaxX, worldMinY, worldMaxY;

	//float x = -700.0f;
	//float y = 370.0f;
	//float w = 180.0f;
	//float h = 140.0f;
	//float dotSize = 10.0f;
	//float worldMinX = -780.0f;
	//float worldMaxX = 780.0f;
	//float worldMinY = -350.0f;
	//float worldMaxY = 2000.0f;
};

void Minimap_Draw(const Minimap& mm, float playerWorldX, float playerWorldY);
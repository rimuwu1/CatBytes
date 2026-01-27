/* Start Header ************************************************************************/
/*!
\file       Platforms.h
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
#pragma once

#include <vector>

struct Platform {
	float x, y, w, h;
};

// global platform mesh
//extern AEGfxVertexList* platformMesh;

//void Platforms_Initialize();

void Platforms_Draw(AEGfxVertexList* mesh, const std::vector<Platform> &platform);

float Get_Highest_Platform_YPos(const std::vector<Platform>& platforms);
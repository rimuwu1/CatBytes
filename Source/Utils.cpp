/* Start Header ************************************************************************/
/*!
\file Utils.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements util functions to create mesh and draw mesh.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "Utils.h"
#include "Player.h"
#include "enemy.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>


namespace util {
	AEGfxVertexList* CreateSquareMesh() {
		AEGfxMeshStart();
		AEGfxTriAdd(
			-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
			0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
			-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
		AEGfxTriAdd(
			0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
			0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
			-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
		return AEGfxMeshEnd();
	}

	void DrawSquare(AEGfxVertexList* mesh, float x, float y, float width, float height, int r, int g, int b) {
		AEMtx33 scale, translate, transform;

		//convert colour
		float red = static_cast<float> (r / 255.0f);
		float green = static_cast<float> (g / 255.0f);
		float blue = static_cast<float> (b / 255.0f);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetColorToMultiply(red, green, blue, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

		//transformations
		AEMtx33Scale(&scale, width, height);
		AEMtx33Trans(&translate, x, y);
		AEMtx33Concat(&transform, &translate, &scale);

		//draw
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}

	void DrawTexturedSquare(AEGfxVertexList* mesh, AEGfxTexture* texture, float x, float y, float width, float height, float opacity) {
		AEMtx33 scale, translate, transform;
		//texture settings
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(opacity);
		AEGfxTextureSet(texture, 0, 0);

		//transformations
		AEMtx33Scale(&scale, width, height);
		AEMtx33Trans(&translate, x, y);
		AEMtx33Concat(&transform, &translate, &scale);
		//draw
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}

	AEGfxVertexList* CreateCircleMesh() {
		AEGfxMeshStart();

		const int segments = 42;
		float radius = 0.5f;  // Unit circle radius
		float angleStep = 2.0f * 3.14159265f / segments;

		for (int i = 0; i < segments; ++i) {
			float angle1 = i * angleStep;
			float angle2 = (i + 1) * angleStep;

			// Add triangle
			AEGfxTriAdd(
				0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,  // Center at (0,0), texture at middle

				cosf(angle1) * radius,           // X1
				sinf(angle1) * radius,           // Y1
				0xFFFFFFFF,
				0.0f,
				0.0f,

				cosf(angle2) * radius,           // X2
				sinf(angle2) * radius,           // Y2
				0xFFFFFFFF,
				0.0f,
				0.0f
			);
		}

		return AEGfxMeshEnd();
	}

	void DrawCircle(AEGfxVertexList* mesh, float x, float y, float diameter, int r, int g, int b) {
		AEMtx33 scale, translate, transform;

		//convert colour
		float red = static_cast<float>(r / 255.0f);
		float green = static_cast<float> (g / 255.0f);
		float blue = static_cast<float> (b / 255.0f);
		AEGfxSetColorToMultiply(red, green, blue, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

		//transformations
		AEMtx33Scale(&scale, diameter, diameter);
		AEMtx33Trans(&translate, x, y);
		AEMtx33Concat(&transform, &translate, &scale);

		//draw
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
	}

	int IsCircleCollided(float circle1_center_x, float circle1_center_y, float circle2_center_x, float circle2_center_y, float diameter1, float diameter2) {
		float radius1 = diameter1 / 2;
		float radius2 = diameter2 / 2;
		float dist_squared2 = (circle2_center_x - circle1_center_x) * (circle2_center_x - circle1_center_x) + (circle2_center_y - circle1_center_y) * (circle2_center_y - circle1_center_y);
		if (dist_squared2 <= (radius1 + radius2) * (radius1 + radius2)) {
			return 1;
		}
		return 0;
	}

}


// ----------------------------------------------------------------------------
// load a float value from a text file
// ----------------------------------------------------------------------------
float LoadFloatFromFile(const char* filename)
{
	std::ifstream file(filename);
	float value = 0.0f;
	file >> value;
	return value;
}

// ----------------------------------------------------------------------------
// load an int value from a text file
// ----------------------------------------------------------------------------

int LoadIntFromFile(const char* filename)
{
	std::ifstream file(filename);
	int value = 1;
	file >> value;
	return value;
}

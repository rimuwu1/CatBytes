#pragma once
#include "AEEngine.h"

float LoadFloatFromFile(const char* filename);
int   LoadIntFromFile(const char* filename);

namespace util {
	AEGfxVertexList* CreateSquareMesh();
	void DrawSquare(AEGfxVertexList* mesh, float x, float y, float width, float height, int r = 255, int g = 0, int b = 0);
	void DrawTexturedSquare(AEGfxVertexList* mesh, AEGfxTexture* texture, float x, float y, float width, float height, float opacity = 1.0f);
	AEGfxVertexList* CreateCircleMesh();
	void DrawCircle(AEGfxVertexList* mesh, float x, float y, float diameter, int r = 255, int g = 0, int b = 0);
	int IsCircleCollided(float circle1_center_x, float circle1_center_y, float circle2_center_x, float circle2_center_y, float diameter1, float diameter2);
}
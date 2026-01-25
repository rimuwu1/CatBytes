/* Start Header ************************************************************************/
/*!
\file Player.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinajijie.wong@digipen.edu
\date January, 23, 2026
\brief This file contains the function definitions for the Player movements, physics,
		input handling, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "Player.h"
#include "Utils.h"

static AEGfxVertexList* playerMesh = nullptr;
static AEGfxTexture* playerTexture = nullptr;//player image

void Player_Init(Player& player, float startX, float startY)
{
	player.pos.x = startX;
	player.pos.y = startY;
	player.vel.x = 0.0f;
	player.vel.y = 0.0f;
	player.width = 50.0f;
	player.height = 50.0f;
	player.grounded = 1;

	if (!playerMesh) // Check for player mesh
	{
		playerMesh = util::CreateSquareMesh();
	}

	//load player texture 
	if (!playerTexture)
	{
		playerTexture = AEGfxTextureLoad("Assets/Images/player.jpg");
	}

}

void Player_Update(Player& player, float dt)
{
	const float GRAVITY = -1200.0f;

	// Gravity
	player.vel.y += GRAVITY * dt;

	// Integrate velocity to position
	player.pos.x += player.vel.x * dt;
	player.pos.y += player.vel.y * dt;
}

void Player_Draw(const Player& player)
{
	/*util::DrawSquare(
		playerMesh,
		player.pos.x,
		player.pos.y,
		player.width,
		player.height,
		0, 0, 255
	);*/

	//test for drawing image instead

	//draw the player using a textured square
	util::DrawTexturedSquare(
		playerMesh,
		playerTexture,
		player.pos.x,
		player.pos.y,
		player.width,
		player.height,
		1.0f//fully opaque
	);

}

// ----------------------------------------------------------------------------
// Releases all dynamically allocated resources used by the player
// this includes the player's mesh and texture, which are shared static resources
// ----------------------------------------------------------------------------
void Player_Free()
{
	//unload the player texture if it was loaded
	if (playerTexture)
	{
		AEGfxTextureUnload(playerTexture);
		playerTexture = nullptr; //prevent accidental reuse 
	}

	//free the player mesh if it exists
	if (playerMesh)
	{
		AEGfxMeshFree(playerMesh);
		playerMesh = nullptr;//mark as freed
	}
}

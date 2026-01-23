#include "pch.h"
#include "Player.h"
#include "Utils.h"

static AEGfxVertexList* playerMesh = nullptr;

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
	util::DrawSquare(
		playerMesh,
		player.pos.x,
		player.pos.y,
		player.width,
		player.height,
		0, 0, 255
	);
}
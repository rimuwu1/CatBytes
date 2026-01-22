#include "pch.h"
#include "Player.h"
#include "Input.h"
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
	const float MOVE_SPEED = 400.0f;
	const float GRAVITY	   = -1200.0f;
	const float JUMP_FORCE = 650.0f;

	player.vel.x = 0.0f;

	// Player Movement
	if (AEInputCheckCurr('A')) {
		player.vel.x -= MOVE_SPEED;
	}
	if (AEInputCheckCurr('D')) {
		player.vel.x += MOVE_SPEED;
	}

	// Player Jumping
	//static int isPressedSpace = 0;
	//int spacePressed = AEInputCheckCurr(AEVK_SPACE);
	if (player.grounded && AEInputCheckCurr(AEVK_SPACE))
	{
		player.vel.y = JUMP_FORCE;
		player.grounded = 0;
	}

	// Gravity
	player.vel.y += GRAVITY * dt;

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
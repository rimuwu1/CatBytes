#pragma once
#include "AEEngine.h"

struct Player
{
	AEVec2 pos;
	AEVec2 vel;
	float width;
	float height;
	int grounded;
};

void Player_Init(Player& player, float startX, float startY);
void Player_Update(Player& player, float dt);
void Player_Draw(const Player& player);

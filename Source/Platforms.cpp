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
#include "MeshManager.h"
#include "Player.h"

void Platforms_Draw(const std::vector<Platform>& platform) {

	// for each Platform named pf in container platform
	/*for (const Platform& pf : platform) {

		MeshManager::Get().DrawSquare(pf.x, pf.y, pf.w, pf.h, 255, 178, 102);

	}*/

	for (const Platform& pf : platform) {
		if (!pf.active) continue;  // skip inactive
		MeshManager::Get().DrawSquare(pf.x, pf.y, pf.w, pf.h, 255, 178, 102);
	}

}

void PlatformButton_Draw(const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms)
{
	for (int i = 0; i < (int)buttons.size(); i++)
	{
		const PlatformButton& btn = buttons[i];
		// green if linked platform is active, red if not
		if (platforms[btn.platformIndex].active)
			MeshManager::Get().DrawSquare(btn.x, btn.y, btn.w, btn.h, 0, 255, 0);
		else
			MeshManager::Get().DrawSquare(btn.x, btn.y, btn.w, btn.h, 255, 0, 0);
	}
}

void PlatformsObstacle_Draw(const std::vector<PlatformObstacle>& obstacles)
{
	for (const PlatformObstacle& obs : obstacles)
	{
		//if (!obs.active) continue;  // skip inactive
		MeshManager::Get().DrawTriangle(obs.x, obs.y, obs.w, obs.h, obs.r); // grey
	}
}

bool CheckObstacleCollision(const Player& player, const std::vector<PlatformObstacle>& obstacle)
{
	for (int i = 0; i < obstacle.size(); i++) {
		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;
		float playerTop = player.pos.y + player.height * 0.5f;
		float playerBottom = player.pos.y - player.height * 0.5f;

		float obsLeft = obstacle[i].x - obstacle[i].w * 0.5f;
		float obsRight = obstacle[i].x + obstacle[i].w * 0.5f;
		float obsTop = obstacle[i].y + obstacle[i].h * 0.5f;
		float obsBottom = obstacle[i].y - obstacle[i].h * 0.5f;

		bool overlapX = (playerRight >= obsLeft) && (playerLeft <= obsRight);
		bool overlapY = (playerTop >= obsBottom) && (playerBottom <= obsTop);

		// If we found a collision, return true immediately
		if (overlapX && overlapY) {
			return true;
		}
	}

	// If we checked EVERY obstacle and found no overlaps, return false.
	return false;
}

void Platforms_OffsetY(std::vector<Platform>& platforms, float offsetY)
{
	for (Platform& pf : platforms)
	{
		pf.y += offsetY;
	}
}

bool Platform_CollisionCheck(Player& player, float& previousY, const std::vector<Platform>& platforms) {

	if (player.vel.y > 0.0f) {

		return false;

	}

	float playerPrevBottom = previousY - player.height * 0.5f;
	float playerCurrBottom = player.pos.y - player.height * 0.5f;

	for (const Platform& pf : platforms)
	{
		float pfLeft = pf.x - pf.w * 0.5f;
		float pfRight = pf.x + pf.w * 0.5f;
		float pfTop = pf.y + pf.h * 0.5f;

		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;

		bool overlapX = (playerRight >= pfLeft) && (playerLeft <= pfRight);
		bool landedThisFrame = (playerPrevBottom >= pfTop) && (playerCurrBottom <= pfTop);

		if (overlapX && landedThisFrame)
		{
			player.pos.y = pfTop + player.height * 0.5f;
			player.vel.y = 0.0f;
			player.grounded = 1;

			previousY = player.pos.y;
			return true;
		}
	}
	return false;

}
void WallCollisionCheck(Player& player, const std::vector<Platform>& wallPlatforms)
{
	for (const Platform& w : wallPlatforms)
	{
		if (!w.active) continue;

		float wLeft = w.x - w.w * 0.5f;
		float wRight = w.x + w.w * 0.5f;
		float wBottom = w.y - w.h * 0.5f;
		float wTop = w.y + w.h * 0.5f;

		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;
		float playerBottom = player.pos.y - player.height * 0.5f;
		float playerTop = player.pos.y + player.height * 0.5f;

		bool overlapX = (playerRight > wLeft) && (playerLeft < wRight);
		bool overlapY = (playerTop > wBottom) && (playerBottom < wTop);

		if (!(overlapX && overlapY))
		{
			continue;
		}

		float pushRight = wRight - playerLeft;
		float pushLeft = playerRight - wLeft;

		if (pushRight < pushLeft)
		{
			player.pos.x += pushRight;
		}
		else
		{
			player.pos.x -= pushLeft;
		}
	}
}

void CheckpointDraw(const std::vector<Checkpoint>& checkpoint){
	for (auto& point : checkpoint) {
		MeshManager::Get().DrawSquare(point.x, point.y, point.w, point.h);
	}
}

bool CheckpointCollisionCheck(const Player& player, const std::vector<Checkpoint>& checkpoint)
{
	for (int i = 0; i < checkpoint.size(); i++) {
		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;
		float playerTop = player.pos.y + player.height * 0.5f;
		float playerBottom = player.pos.y - player.height * 0.5f;

		float obsLeft = checkpoint[i].x - checkpoint[i].w * 0.5f;
		float obsRight = checkpoint[i].x + checkpoint[i].w * 0.5f;
		float obsTop = checkpoint[i].y + checkpoint[i].h * 0.5f;
		float obsBottom = checkpoint[i].y - checkpoint[i].h * 0.5f;

		bool overlapX = (playerRight >= obsLeft) && (playerLeft <= obsRight);
		bool overlapY = (playerTop >= obsBottom) && (playerBottom <= obsTop);

		// If we found a collision, return true immediately
		if (overlapX && overlapY) {
			return true;
		}
	}
	return false;
}
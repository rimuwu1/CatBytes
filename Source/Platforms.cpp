/* Start Header ************************************************************************/
/*!
\file       Platforms.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
			Joash Ng, joash.ng, 2502780
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
			joash.ng@digipen.edu
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
#include "TextureManager.h"
#include "SpriteSheet.h"
#include "Fonts.h"
#include "AEEngine.h"
#include "Camera.h"
#include <memory>




void Platforms_Draw(const std::vector<Platform>& platforms, AEGfxTexture* leftTex, AEGfxTexture* midTex, AEGfxTexture* rightTex) {

	static SpriteSheet hoverAnim("Assets/Images/HoverSheet.png", 1, 4, 0, 2.0f);
	
	const float capWidth = 32.0f;   // width of left/right cap in world units
	
	float dt = (float)AEFrameRateControllerGetFrameTime();
	hoverAnim.Update(dt);

	for (const Platform& pf : platforms) {
		if (!pf.active) continue;

		// Left cap position
		float leftX = pf.x - pf.w / 2 + capWidth / 2;
		MeshManager::Get().DrawTexturedSquare(leftTex, leftX, pf.y, capWidth, pf.h);

		// Right cap position
		float rightX = pf.x + pf.w / 2 - capWidth / 2;
		MeshManager::Get().DrawTexturedSquare(rightTex, rightX, pf.y, capWidth, pf.h);

		// Middle section: a single section that stretches between the caps
		float midStartX = leftX + capWidth / 2;            // left edge of middle
		float midEndX = rightX - capWidth / 2;           // right edge of middle
		float midWidth = midEndX - midStartX;

		if (midWidth > 0.0f) {
			float midCenterX = (midStartX + midEndX) * 0.5f;
			MeshManager::Get().DrawTexturedSquare(midTex,
				midCenterX, pf.y,
				midWidth, pf.h);
			MeshManager::Get().DrawSpriteSheet(hoverAnim, midCenterX, pf.y - 40.0f, midWidth, pf.h);
		}
	}
}

void PlatformButton_Draw(const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms, const Player& player)
{
	static SpriteSheet platformSwitch("Assets/Images/buttonSheet2.png", 3, 4, 9, 0.1f);

	// clips
	static bool addClips = false;

	if (!addClips) {
		platformSwitch.AddClip("off", 8, 8, 0.1f, false);			// off state - row 1 column 5
		platformSwitch.AddClip("transition", 0, 3, 1.0f, false);	// off -> on - row 1 columns 1-4
		platformSwitch.AddClip("on", 4, 7, 0.1f, true);				// on state - row 2 columns 1 - 4

		platformSwitch.Play("off");

		addClips = true;
	}

	float dt = (float)AEFrameRateControllerGetFrameTime();
	platformSwitch.Update(dt);

	for (const PlatformButton& btn : buttons) {
		bool isActive = false;

		if (!btn.platformIndices.empty()) {
			int index = btn.platformIndices[0];

			if (index >= 0 && index < (int)platforms.size()) {
				isActive = platforms[index].active;
			}
		}

		// initialize based on first draw's state
		if (!btn.spriteInitialized)
		{
			if (isActive) 
			{
				platformSwitch.Play("on");
			}
			else 
			{
				platformSwitch.Play("off");
			}

			btn.prevState = isActive;
			btn.spriteInitialized = true;
		}
		
		// change clip when state changes
		if (isActive != btn.prevState) 
		{
			if (isActive)
			{
				platformSwitch.Play("transition");
			}
			else
			{
				platformSwitch.Play("off");
			}

			btn.prevState = isActive;
			
		}
		
		// move to on state once transition finishes
		if (platformSwitch.GetCurrentClip() == "transition" && !platformSwitch.IsPlaying()) {
			platformSwitch.Play("on");
		}

		MeshManager::Get().DrawSpriteSheet(platformSwitch, btn.x, btn.y, btn.w, btn.h);

		// ----------------------------------------------------------------------------
		// text for when player is near switch
		
		// check if player is near/overlapping switch
		float btnLeft = btn.x - btn.w * 0.5f;
		float btnRight = btn.x + btn.w * 0.5f;
		float btnTop = btn.y + btn.h * 0.5f;
		float btnBottom = btn.y - btn.h * 0.5f;
		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;
		float playerBottom = player.pos.y - player.height * 0.5f;
		float playerTop = player.pos.y + player.height * 0.5f;

		bool overlapX = (playerRight >= btnLeft) && (playerLeft <= btnRight);
		bool overlapY = (playerTop >= btnBottom) && (playerBottom <= btnTop);
		bool inRange = overlapX && overlapY;

		if (inRange)
		{
			float windowWidth = (float)AEGfxGetWindowWidth();
			float windowHeight = (float)AEGfxGetWindowHeight();

			float screenX = (btn.x - globalCam.x) / (windowWidth * 0.5f);
			float screenY = (btn.y + btn.h + 20.0f - globalCam.y) / (windowHeight * 0.5f);

			// shift text towards left
			if (screenX > 0.5f)
			{
				screenX = 0.5;
			}

			// shift text towards right
			if (screenX < -0.9f)
			{
				screenX = -0.9f;
			}

			char text[50];

			if (isActive)
			{
				strcpy_s(text, "Press E to turn off platforms");
			}
			else 
			{
				strcpy_s(text, "Press E to turn on platforms");
			}

			AEGfxPrint(g_FontSmall, text, screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

}

void PlatformsObstacle_Draw(const std::vector<PlatformObstacle>& obstacles)
{
	for (const PlatformObstacle& obs : obstacles)
	{
		//if (!obs.active) continue;  // skip inactive
		MeshManager::Get().DrawTexturedSquare(TextureManager::Get().LoadTexture("Assets/Images/spikeObstacle.png"), obs.x, obs.y, obs.w, obs.h);
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

void CheckpointDraw(const std::vector<Checkpoint>& checkpoint, const Player& player){
	static SpriteSheet checkpointAnim("Assets/Images/checkpointSheet.png", 1, 10, 0, .1f);
	float dt = (float)AEFrameRateControllerGetFrameTime();
	checkpointAnim.Update(dt);

	for (auto& point : checkpoint) {
		MeshManager::Get().DrawSpriteSheet(checkpointAnim, point.x, point.y, point.w, point.h);

		float playerLeft = player.pos.x - player.width * 0.5f;
		float playerRight = player.pos.x + player.width * 0.5f;
		float playerTop = player.pos.y + player.height * 0.5f;
		float playerBottom = player.pos.y - player.height * 0.5f;

		float rangeLeft = point.x - point.w * 1.0f;
		float rangeRight = point.x + point.w * 1.0f;
		float rangeTop = point.y + point.h * 1.0f;
		float rangeBottom = point.y - point.h * 1.0f;

		bool inRangeX = (playerRight >= rangeLeft) && (playerLeft <= rangeRight);
		bool inRangeY = (playerTop >= rangeBottom) && (playerBottom <= rangeTop);

		if (inRangeX && inRangeY) {
			float windowWidth = (float)AEGfxGetWindowWidth();
			float windowHeight = (float)AEGfxGetWindowHeight();

			float screenX = (point.x - globalCam.x) / (windowWidth * 0.5f);
			float screenY = (point.y + point.h + 20.0f - globalCam.y) / (windowHeight * 0.5f);

			if (screenX > 0.5f) {
				screenX = 0.5f;
			}
			if (screenX < -0.9f) {
				screenX = -0.9f;
			}

			AEGfxPrint(g_FontSmall, "Press E to save game", screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

void CheckpointCollisionCheck(const Player& player, const std::vector<Checkpoint>& checkpoint, bool& checkpointHit, bool& checkpointInRange)
{
	checkpointHit = false;
	checkpointInRange = false;
	const Checkpoint* nearestInRange = nullptr;

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

		if (overlapX && overlapY) {
			checkpointHit = true;
		}

		float rangeLeft = checkpoint[i].x - checkpoint[i].w * 1.0f;
		float rangeRight = checkpoint[i].x + checkpoint[i].w * 1.0f;
		float rangeTop = checkpoint[i].y + checkpoint[i].h * 1.0f;
		float rangeBottom = checkpoint[i].y - checkpoint[i].h * 1.0f;

		bool inRangeX = (playerRight >= rangeLeft) && (playerLeft <= rangeRight);
		bool inRangeY = (playerTop >= rangeBottom) && (playerBottom <= rangeTop);

		if (inRangeX && inRangeY && !nearestInRange) {
			checkpointInRange = true;
			nearestInRange = &checkpoint[i];
		}
	}
}
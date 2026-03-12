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
#include <algorithm>


//MOVED TO ENV MANAGER

//void Platforms_Draw(const std::vector<Platform>& platforms,
//	AEGfxTexture* leftTex, AEGfxTexture* midTex, AEGfxTexture* rightTex)
//{
//	static SpriteSheet hoverAnim("Assets/Images/HoverSheet.png", 1, 4, 0, 2.0f);
//	const float capWidth = 32.0f;
//
//	float dt = (float)AEFrameRateControllerGetFrameTime();
//	hoverAnim.Update(dt);
//
//	MeshManager& mm = MeshManager::Get();
//
//	// ---- Collect all sprites, grouped by texture ----
//	struct Entry { AEGfxTexture* tex; float uvW, uvH, x, y, w, h, uvOffX, uvOffY; };
//	std::vector<Entry> sprites;
//	sprites.reserve(platforms.size() * 4);
//
//	for (const Platform& pf : platforms) {
//		if (!pf.active) continue;
//
//		float leftX = pf.x - pf.w * 0.5f + capWidth * 0.5f;
//		float rightX = pf.x + pf.w * 0.5f - capWidth * 0.5f;
//
//		sprites.push_back({ leftTex,  1.f, 1.f, leftX,  pf.y, capWidth, pf.h, 0.f, 0.f });
//		sprites.push_back({ rightTex, 1.f, 1.f, rightX, pf.y, capWidth, pf.h, 0.f, 0.f });
//
//		float midStartX = leftX + capWidth * 0.5f;
//		float midEndX = rightX - capWidth * 0.5f;
//		float midWidth = midEndX - midStartX;
//
//		if (midWidth > 0.0f) {
//			float midCX = (midStartX + midEndX) * 0.5f;
//			sprites.push_back({ midTex, 1.f, 1.f, midCX, pf.y, midWidth, pf.h, 0.f, 0.f });
//			sprites.push_back({
//				hoverAnim.GetTexture(),
//				hoverAnim.GetSpriteUVWidth(), hoverAnim.GetSpriteUVHeight(),
//				midCX, pf.y - 40.0f, midWidth, pf.h,
//				hoverAnim.GetUVOffsetX(), hoverAnim.GetUVOffsetY()
//				});
//		}
//	}
//
//	// ---- Sort by texture so same-texture sprites batch together ----
//	std::sort(sprites.begin(), sprites.end(),
//		[](const Entry& a, const Entry& b) { return a.tex < b.tex; });
//
//	// ---- One BeginBatch/EndBatch per texture group ----
//	size_t i = 0;
//	while (i < sprites.size()) {
//		const Entry& first = sprites[i];
//		mm.BeginBatch(first.tex, first.uvW, first.uvH);
//		do {
//			SpriteBatchItem item{};
//			item.x = sprites[i].x;
//			item.y = sprites[i].y;
//			item.width = sprites[i].w;
//			item.height = sprites[i].h;
//			item.uvOffsetX = sprites[i].uvOffX;
//			item.uvOffsetY = sprites[i].uvOffY;
//			item.opacity = 1.0f;
//			item.rotation = 0.0f;
//			mm.QueueSprite(item);
//			++i;
//		} while (i < sprites.size() && sprites[i].tex == first.tex);
//		mm.EndBatch();
//	}
//}
//
void PlatformButton_Draw(std::vector<PlatformButton>& buttons,
	const std::vector<Platform>& platforms,
	const Player& player)
{
	float dt = (float)AEFrameRateControllerGetFrameTime();

	for (auto& btn : buttons) {   // now non?const, so we can modify
		if (!btn.buttonSprite) continue;

		bool isActive = false;
		if (!btn.platformIndices.empty()) {
			int idx = btn.platformIndices[0];
			if (idx >= 0 && idx < (int)platforms.size())
				isActive = platforms[idx].active;
		}

		// First?draw initialisation
		if (!btn.spriteInitialized) {
			btn.buttonSprite->Play(isActive ? "on" : "off");
			btn.prevState = isActive;
			btn.spriteInitialized = true;
		}

		// State change
		if (isActive != btn.prevState) {
			if (isActive)
				btn.buttonSprite->Play("transition");
			else
				btn.buttonSprite->Play("off");
			btn.prevState = isActive;
		}

		btn.buttonSprite->Update(dt);

		if (btn.buttonSprite->GetCurrentClip() == "transition" && !btn.buttonSprite->IsPlaying())
			btn.buttonSprite->Play("on");

		MeshManager::Get().DrawSpriteSheet(*btn.buttonSprite, btn.x, btn.y, btn.w, btn.h);

		// ----- "Press E" prompt (unchanged) -----
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
		if (overlapX && overlapY) {
			float windowWidth = (float)AEGfxGetWindowWidth();
			float windowHeight = (float)AEGfxGetWindowHeight();
			float screenX = (btn.x - globalCam.x) / (windowWidth * 0.5f);
			float screenY = (btn.y + btn.h + 20.0f - globalCam.y) / (windowHeight * 0.5f);
			if (screenX > 0.5f)  screenX = 0.5f;
			if (screenX < -0.9f) screenX = -0.9f;
			const char* msg = isActive ? "Press E to turn off platforms" : "Press E to turn on platforms";
			AEGfxPrint(g_FontSmall, msg, screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}
//
//void PlatformsObstacle_Draw(const std::vector<PlatformObstacle>& obstacles)
//{
//	static AEGfxTexture* spikeTex = TextureManager::Get().LoadTexture("Assets/Images/spikeObstacle.png");
//	MeshManager& mm = MeshManager::Get();
//
//	mm.BeginBatch(spikeTex, 1.0f, 1.0f);
//	for (const PlatformObstacle& obs : obstacles)
//	{
//		//if (!obs.active) continue;  // skip inactive
//		SpriteBatchItem item{};
//		item.x = obs.x;
//		item.y = obs.y;
//		item.width = obs.w;
//		item.height = obs.h;
//		item.uvOffsetX = 0.0f;
//		item.uvOffsetY = 0.0f;
//		item.opacity = 1.0f;
//		item.rotation = 0.0f;
//		mm.QueueSprite(item);
//	}
//	mm.EndBatch();
//}

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

//void CheckpointDraw(const std::vector<Checkpoint>& checkpoint, const Player& player){
//	static SpriteSheet checkpointAnim("Assets/Images/checkpointSheet.png", 1, 10, 0, .1f);
//	float dt = (float)AEFrameRateControllerGetFrameTime();
//	checkpointAnim.Update(dt);
//
//	MeshManager& mm = MeshManager::Get();
//
//	mm.BeginBatch(checkpointAnim.GetTexture(), checkpointAnim.GetSpriteUVWidth(), checkpointAnim.GetSpriteUVHeight());
//	for (auto& point : checkpoint) {
//		SpriteBatchItem item{};
//		item.x = point.x;
//		item.y = point.y;
//		item.width = point.w;
//		item.height = point.h;
//		item.uvOffsetX = checkpointAnim.GetUVOffsetX();
//		item.uvOffsetY = checkpointAnim.GetUVOffsetY();
//		item.opacity = 1.0f;
//		item.rotation = 0.0f;
//		mm.QueueSprite(item);
//
//		// Check for "Press E" text (must remain outside batch)
//		float playerLeft = player.pos.x - player.width * 0.5f;
//		float playerRight = player.pos.x + player.width * 0.5f;
//		float playerTop = player.pos.y + player.height * 0.5f;
//		float playerBottom = player.pos.y - player.height * 0.5f;
//
//		float rangeLeft = point.x - point.w * 1.0f;
//		float rangeRight = point.x + point.w * 1.0f;
//		float rangeTop = point.y + point.h * 1.0f;
//		float rangeBottom = point.y - point.h * 1.0f;
//
//		bool inRangeX = (playerRight >= rangeLeft) && (playerLeft <= rangeRight);
//		bool inRangeY = (playerTop >= rangeBottom) && (playerBottom <= rangeTop);
//
//		if (inRangeX && inRangeY) {
//			float windowWidth = (float)AEGfxGetWindowWidth();
//			float windowHeight = (float)AEGfxGetWindowHeight();
//
//			float screenX = (point.x - globalCam.x) / (windowWidth * 0.5f);
//			float screenY = (point.y + point.h + 20.0f - globalCam.y) / (windowHeight * 0.5f);
//
//			if (screenX > 0.5f) {
//				screenX = 0.5f;
//			}
//			if (screenX < -0.9f) {
//				screenX = -0.9f;
//			}
//
//			AEGfxPrint(g_FontSmall, "Press E to save game", screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
//		}
//	}
//	mm.EndBatch();
//}

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
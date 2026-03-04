/* Start Header ************************************************************************/
/*!
\file	HUD.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par	kerwinjiajie.wong@digipen.edu
\date	February, 19, 2026
\brief	This file contains the function declarations for the in-game HUD, defining UI
		elements and their interfaces.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include "AEEngine.h"
#include "Player.h"
#include <memory>
#include <rapidjson/document.h>

#include <array>
#include <string>

class MeshManager;
class SpriteSheet;

struct Player;

class HUD
{
public:
	bool IsPauseButtonClicked(float camX, float camY) const;

	void InitFromConfig(const rapidjson::Value& doc);
	void Update(float /*dt*/, const Player& player);
	void Draw(MeshManager& meshManager, float camX, float camY, PlayerWeapon weapon) const;

	bool IsActive() const { return hudActive; }

private:
	bool hudActive = true;

	// ---- Hearts UI ---- //
	struct Hearts
	{
		bool active = true;

		float offsetX = 680.0f;
		float offsetY = 410.0f;
		float width = 128.0f;
		float height = 128.0f;

		std::unique_ptr<SpriteSheet> heartsSheet;

		int lastHeartsState = -1;
	};

	Hearts hearts;

	// ---- Weapon Switch UI ---- //
	struct WeaponSwitch
	{
		bool active = true;

		float offsetX = 650.0f;
		float offsetY = -420.0f;

		float slotSize = 100.0f;
		float iconSize = 60.0f;

		AEGfxTexture* slotTexture = nullptr;
		AEGfxTexture* meleeIcon = nullptr;
		AEGfxTexture* gunIcon = nullptr;

		bool ready = false;
	};

	WeaponSwitch weaponSwitch;

	// ---- Progress Bar UI ---- //
	struct ProgressBar
	{
		bool active = false;

		float offsetX = -650.0f;
		float offsetY = 0.0f;
		float width = 16.0f;
		float height = 80.0f;

		float minY = 0.0f;
		float maxY = 7500.0f;

		float gap = 0.0f;
		float paddingX = 2.0f;
		float paddingY = 2.0f;
		float overlapY = 1.0f;

		std::array<float, 3> segmentEndY{ 1900.0f, 4550.0f, 7500.0f };

		float trackerRadius = 5.0f;
		int trackerR = 255, trackerG = 255, trackerB = 255;

		//std::unique_ptr<SpriteSheet> pbarSheet;
		AEGfxTexture* texture = nullptr;
		bool pbarReady = false;
	};

	ProgressBar progressBar;
	float pbarPlayerY = 0.0f;

	// ---- Pause Button UI ---- //
	struct PauseButton
	{
		bool active = true;
		float offsetX = -750.0f;
		float offsetY = 420.0f;
		float width = 32.0f;
		float height = 32.0f;

		//std::unique_ptr<SpriteSheet> pauseSheet;
		AEGfxTexture* texture = nullptr;
		bool ready = false;
	};

	PauseButton pauseButton;

private:
	static int ClampHeartsStateFromPlayer(const Player& player);
	void ApplyHeartsState(int state);

	static float ClampProgressBar(float v);
	static float SegmentProgress(float y, float a, float b);

	void InitHeartsFromConfig(const rapidjson::Value& uiJson);
	void InitWeaponSwitchFromConfig(const rapidjson::Value& uiJson);
	void InitProgressBarFromConfig(const rapidjson::Value& uiJson);
	void InitPauseButtonFromConfig(const rapidjson::Value& uiJson);

	void DrawHearts(float camX, float camY) const;
	void DrawWeaponSwitch(float camX, float camY, PlayerWeapon weapon) const;
	void DrawProgressBar(MeshManager& meshManager, float camX, float camY) const;
	void DrawPauseButton(float camX, float camY) const;
};
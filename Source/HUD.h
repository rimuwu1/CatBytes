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
#include "Buff.h"
#include "Camera.h"
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
	int IsInventorySlotClicked(float camX, float camY) const;  // Returns 0-2 for slot index, -1 if no click
	PlayerWeapon IsWeaponSlotClicked(float camX, float camY) const;  // Returns weapon clicked, or NONE
	bool IsAnyUIElementClicked(float camX, float camY) const;  // Returns true if any UI element is clicked

	void AddBuffToInventory(BuffType buffType);
	void UseBuffFromInventory(Player& player, int slot);
	int FindBuffSlot(BuffType buffType) const;

	void UpdateBuffBar(const Player& player);
	void DrawBuffBar(float camX, float camY) const;

	void InitFromConfig(const rapidjson::Value& doc);
	void Update(float dt, const Player& player, PlayerWeapon weapon);
	void Draw(MeshManager& meshManager, float camX, float camY, PlayerWeapon weapon) const;

	bool IsActive() const { return hudActive; }

	void UpdateButtonStates(float dt, float camX, float camY);
	void UpdatePressTimers(float dt);
	void TriggerWeaponSlotPress(int slotIndex);
	void TriggerInventorySlotPress(int slotIndex);
	void TriggerPauseButtonPress();

private:
	bool hudActive = true;

	// ---- Button State Tracking ---- //
	struct UIButtonState
	{
		bool isHovered = false;
		float hoverAlpha = 0.0f;      // 0.0 to 1.0 for fade-in/out
		bool isPressed = false;
		float pressTimer = 0.0f;      // counts down during press animation
	};

	void TriggerButtonPress(UIButtonState& state);

	UIButtonState pauseButtonState;
	UIButtonState inventorySlotStates[3];
	UIButtonState weaponSlotStates[2];

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

		std::unique_ptr<SpriteSheet> meleeSheet;
		std::unique_ptr<SpriteSheet> gunSheet;

		PlayerWeapon lastWeapon = PlayerWeapon::NONE;

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
		float maxY = 9600.0f;

		float gap = 0.0f;
		float paddingX = 2.0f;
		float paddingY = 2.0f;
		float overlapY = 1.0f;

		std::array<float, 3> segmentEndY{ 1900.0f, 4650.0f, 9600.0f };

		float trackerRadius = 5.0f;
		int trackerR = 255, trackerG = 255, trackerB = 255;

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

	// ---- Inventory UI ---- //
	struct Inventory
	{
		bool active = true;
		float offsetX = -650.0f;
		float offsetY = -300.0f;
		float width = 256.0f;
		float height = 128.0f;
		float slotSize = 80.0f;   // individual slot size
		float iconSize = 60.0f;   // buff icon size inside slot

		AEGfxTexture* inventoryTexture = nullptr;
		AEGfxTexture* slotTexture = nullptr;
		AEGfxTexture* shieldTexture = nullptr;
		AEGfxTexture* fullHpTexture = nullptr;
		AEGfxTexture* dashTexture = nullptr;

		// Tracks buff type for each slot
		std::array<BuffType, 3> slots { 
			BuffType::NONE, BuffType::NONE, BuffType::NONE 
		};
		std::array<int, 3> counts{
			0, 0, 0
		};

		bool ready = false;
	};

	Inventory inventory;

	// ---- Buff Bar UI ---- //
	struct BuffBarSlot
	{
		BuffType type = BuffType::NONE;
		float duration = 0.0f;
		float timer = 0.0f;
		int uses = 0;
	};

	struct BuffBar
	{
		bool active = true;
		float x = -200.0f;
		float y = 400.0f;
		float iconSize = 48.0f;
		float gap = 8.0f;

		static constexpr int MAX_SLOTS = 3;
		std::array<BuffBarSlot, MAX_SLOTS> slots{};
		int slotCount = 0;

		AEGfxTexture* badgeTexture = nullptr;
	};

	BuffBar buffBar;

private:
	static int ClampHeartsStateFromPlayer(const Player& player);
	void ApplyHeartsState(int state);

	static float ClampProgressBar(float v);
	static float SegmentProgress(float y, float a, float b);

	void InitHeartsFromConfig(const rapidjson::Value& uiJson);
	void InitWeaponSwitchFromConfig(const rapidjson::Value& uiJson);
	void InitProgressBarFromConfig(const rapidjson::Value& uiJson);
	void InitPauseButtonFromConfig(const rapidjson::Value& uiJson);
	void InitInventoryFromConfig(const rapidjson::Value& uiJson);
	void InitBuffBarFromConfig(const rapidjson::Value& uiJson);

	void DrawHearts(float camX, float camY) const;
	void DrawWeaponSwitch(float camX, float camY, PlayerWeapon weapon) const;
	void DrawProgressBar(MeshManager& meshManager, float camX, float camY) const;
	void DrawPauseButton(float camX, float camY) const;
	void DrawInventory(float camX, float camY) const;
};
/* Start Header ************************************************************************/
/*!
\file	HUD.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
        Joash ng, joash.ng, 2502780
\par	kerwinjiajie.wong@digipen.edu
        joash.ng@digipen.edu
\date	February, 19, 2026
\brief	This file contains the function definitions for the in-game HUD, handling UI
		initialisation from the config and rendering on-screen elements based on
		the current game state.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "HUD.h"
#include "MeshManager.h"
#include "EnvironmentManager.h"
#include "SpriteSheet.h"
#include "Player.h"
#include "Fonts.h"

#include <rapidjson/document.h>

// ---- Helper functions (General) ---- //
// Converts mouse position to in-game coordinates
static void MousePosition(float camX, float camY, float& outX, float& outY)
{
	int mx, my;
	AEInputGetCursorPosition(&mx, &my);

	const float windowWidth = (float)AEGfxGetWindowWidth();
	const float windowHeight = (float)AEGfxGetWindowHeight();

	const float left = camX - windowWidth * 0.5f;
	const float top = camY + windowHeight * 0.5f;

	outX = left + (float)mx;
	outY = top - (float)my;
}

// ---- Helper functions for Hearts UI ---- 
// Clamps hearts (0 to 3)
int HUD::ClampHeartsStateFromPlayer(const Player& player)
{
	int hpInt = static_cast<int>(player.hp);
	if (hpInt < 0)
	{
		hpInt = 0;
	}
	if (hpInt > 5)
	{
		hpInt = 5;
	}

	return hpInt;
}

// Applies individual frame to the equivalent heart state
void HUD::ApplyHeartsState(int state)
{
	if (!hearts.heartsSheet)
	{
		return;
	}
	if (state == hearts.lastHeartsState)
	{
		return;
	}

	hearts.lastHeartsState = state;

	switch (state)
	{
	case 5: hearts.heartsSheet->Play("hp5", true);
		break;
	case 4: hearts.heartsSheet->Play("hp4", true);
		break;
	case 3: hearts.heartsSheet->Play("hp3", true);
		break;
	case 2: hearts.heartsSheet->Play("hp2", true);
		break;
	case 1: hearts.heartsSheet->Play("hp1", true);
		break;
	default: hearts.heartsSheet->Play("hp0", true);
		break;
	}

	hearts.heartsSheet->Stop();
}

// ---- Helper functions for Progress Bar UI ---- //
// Clamps progress bar (0 to 1)
float HUD::ClampProgressBar(float v)
{
	if (v < 0.0f)
	{
		return 0.0f;
	}
	if (v > 1.0f)
	{
		return 1.0f;
	}

	return v;
}

// Tracks player progress through individual segments
float HUD::SegmentProgress(float y, float a, float b)
{
	if (b <= a)
	{
		return 0.0f;
	}

	return ClampProgressBar((y - a) / (b - a));
}

// ------------------------------------------------------------------------
// ---- HUD config ---- //
void HUD::InitFromConfig(const rapidjson::Value& doc)
{
	hudActive = true;

	// Hearts UI
	hearts.active = true;

	// Progress Bar UI
	progressBar = ProgressBar();
	progressBar.active = false;

	if (!doc.HasMember("ui"))
	{
		return;
	}
	const rapidjson::Value& uiJson = doc["ui"];

	InitHeartsFromConfig(uiJson);
	InitWeaponSwitchFromConfig(uiJson);
	InitProgressBarFromConfig(uiJson);
	InitPauseButtonFromConfig(uiJson);
	InitInventoryFromConfig(uiJson);
	InitBuffBarFromConfig(uiJson);
}

// ---- Hearts config ---- //
void HUD::InitHeartsFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("hearts"))
	{
		return;
	}

	const rapidjson::Value& heartsJson = uiJson["hearts"];

	if (heartsJson.HasMember("active"))
	{
		hearts.active = heartsJson["active"].GetBool();
	}

	if (heartsJson.HasMember("x"))
	{
		hearts.offsetX = heartsJson["x"].GetFloat();
	}
	if (heartsJson.HasMember("y"))
	{
		hearts.offsetY = heartsJson["y"].GetFloat();
	}
	if (heartsJson.HasMember("w"))
	{
		hearts.width = heartsJson["w"].GetFloat();
	}
	if (heartsJson.HasMember("h"))
	{
		hearts.height = heartsJson["h"].GetFloat();
	}

	if (heartsJson.HasMember("animations"))
	{
		const rapidjson::Value& anims = heartsJson["animations"];

		if (anims.HasMember("file") && anims.HasMember("rows") && anims.HasMember("cols"))
		{
			hearts.heartsSheet.reset(new SpriteSheet(
				anims["file"].GetString(),
				(u32)anims["rows"].GetInt(),
				(u32)anims["cols"].GetInt()
			));

			if (anims.HasMember("clips"))
			{
				const rapidjson::Value& clips = anims["clips"];
				for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
				{
					const rapidjson::Value& c = clips[i];
					hearts.heartsSheet->AddClip(
						c["name"].GetString(),
						(u32)c["start"].GetInt(),
						(u32)c["end"].GetInt(),
						c["speed"].GetFloat(),
						c["loop"].GetBool()
					);
				}
			}

			hearts.heartsSheet->Play("hp5", true);
			hearts.heartsSheet->Stop();
			hearts.lastHeartsState = 5;
		}
	}

}

// ---- Weapon Switch config ---- //
void HUD::InitWeaponSwitchFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("weaponSwitch"))
	{
		return;
	}

	const rapidjson::Value& weapon = uiJson["weaponSwitch"];

	if (weapon.HasMember("active"))
	{
		weaponSwitch.active = weapon["active"].GetBool();
	}
	if (weapon.HasMember("x"))
	{
		weaponSwitch.offsetX = weapon["x"].GetFloat();
	}
	if (weapon.HasMember("y"))
	{
		weaponSwitch.offsetY = weapon["y"].GetFloat();
	}
	if (weapon.HasMember("slotSize"))
	{
		weaponSwitch.slotSize = weapon["slotSize"].GetFloat();
	}
	if (weapon.HasMember("iconSize"))
	{
		weaponSwitch.iconSize = weapon["iconSize"].GetFloat();
	}
	if (weapon.HasMember("slotTexture"))
	{
		weaponSwitch.slotTexture =
			TextureManager::Get().LoadTexture(weapon["slotTexture"].GetString());
	}

	// ---- Animations ---- //
	// Melee selected
	if (weapon.HasMember("meleeSelected"))
	{
		const rapidjson::Value& anims = weapon["meleeSelected"];

		if (anims.HasMember("file") && anims.HasMember("rows") && anims.HasMember("cols"))
		{
			weaponSwitch.meleeSheet.reset(new SpriteSheet(
				anims["file"].GetString(),
				(u32)anims["rows"].GetInt(),
				(u32)anims["cols"].GetInt()
			));

			const rapidjson::Value& clips = anims["clips"];
			for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
			{
				const rapidjson::Value& c = clips[i];

				weaponSwitch.meleeSheet->AddClip(
					c["name"].GetString(),
					(u32)c["start"].GetInt(),
					(u32)c["end"].GetInt(),
					c["speed"].GetFloat(),
					c["loop"].GetBool()
				);
			}

			weaponSwitch.meleeSheet->Play("selected", true);
			weaponSwitch.meleeSheet->Stop();
		}
	}

	// Gun selected
	if (weapon.HasMember("gunSelected"))
	{
		const rapidjson::Value& anims = weapon["gunSelected"];

		if (anims.HasMember("file") && anims.HasMember("rows") && anims.HasMember("cols"))
		{
			weaponSwitch.gunSheet.reset(new SpriteSheet(
				anims["file"].GetString(),
				(u32)anims["rows"].GetInt(),
				(u32)anims["cols"].GetInt()
			));

			const rapidjson::Value& clips = anims["clips"];
			for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
			{
				const rapidjson::Value& c = clips[i];

				weaponSwitch.gunSheet->AddClip(
					c["name"].GetString(),
					(u32)c["start"].GetInt(),
					(u32)c["end"].GetInt(),
					c["speed"].GetFloat(),
					c["loop"].GetBool()
				);
			}

			weaponSwitch.gunSheet->Play("selected", true);
			weaponSwitch.gunSheet->Stop();
		}
	}

	weaponSwitch.ready = true;
}

// ---- ProgressBar config ---- //
void HUD::InitProgressBarFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("progressBar"))
	{
		return;
	}

	const rapidjson::Value& pbar = uiJson["progressBar"];

	if (pbar.HasMember("active"))
	{
		progressBar.active = pbar["active"].GetBool();
	}
	if (pbar.HasMember("x"))
	{
		progressBar.offsetX = pbar["x"].GetFloat();
	}
	if (pbar.HasMember("y"))
	{
		progressBar.offsetY = pbar["y"].GetFloat();
	}
	if (pbar.HasMember("w"))
	{
		progressBar.width = pbar["w"].GetFloat();
	}
	if (pbar.HasMember("h"))
	{
		progressBar.height = pbar["h"].GetFloat();
	}

	if (pbar.HasMember("minY"))
	{
		progressBar.minY = pbar["minY"].GetFloat();
	}
	if (pbar.HasMember("maxY"))
	{
		progressBar.maxY = pbar["maxY"].GetFloat();
	}

	if (pbar.HasMember("gap"))
	{
		progressBar.gap = pbar["gap"].GetFloat();
	}
	if (pbar.HasMember("paddingX"))
	{
		progressBar.paddingX = pbar["paddingX"].GetFloat();
	}
	if (pbar.HasMember("paddingY"))
	{
		progressBar.paddingY = pbar["paddingY"].GetFloat();
	}
	if (pbar.HasMember("overlapY"))
	{
		progressBar.overlapY = pbar["overlapY"].GetFloat();
	}

	if (pbar.HasMember("trackerRadius"))
	{
		progressBar.trackerRadius = pbar["trackerRadius"].GetFloat();
	}

	if (pbar.HasMember("segments") && pbar["segments"].IsArray())
	{
		const rapidjson::Value& segs = pbar["segments"];
		const int count = (segs.Size() < 3) ? (int)segs.Size() : 3;

		for (int i = 0; i < count; i++)
		{
			const rapidjson::Value& s = segs[i];
			if (s.HasMember("endY"))
			{
				progressBar.segmentEndY[i] = s["endY"].GetFloat();
			}
		}
	}

	progressBar.segmentEndY[2] = progressBar.maxY;

	if (pbar.HasMember("texture"))
	{
		progressBar.texture = TextureManager::Get().LoadTexture(pbar["texture"].GetString());
		progressBar.pbarReady = true;
	}

}

// ---- Pause Button config ---- //
void HUD::InitPauseButtonFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("pauseButton"))
	{
		return;
	}

	const rapidjson::Value& pauseBtn = uiJson["pauseButton"];

	if (pauseBtn.HasMember("active"))
	{
		pauseButton.active = pauseBtn["active"].GetBool();
	}
	if (pauseBtn.HasMember("x"))
	{
		pauseButton.offsetX = pauseBtn["x"].GetFloat();
	}
	if (pauseBtn.HasMember("y"))
	{
		pauseButton.offsetY = pauseBtn["y"].GetFloat();
	}
	if (pauseBtn.HasMember("w"))
	{
		pauseButton.width = pauseBtn["w"].GetFloat();
	}
	if (pauseBtn.HasMember("h"))
	{
		pauseButton.height = pauseBtn["h"].GetFloat();
	}

	if (pauseBtn.HasMember("texture"))
	{
		pauseButton.texture = TextureManager::Get().LoadTexture(pauseBtn["texture"].GetString());
		pauseButton.ready = true;
	}

}

// ---- Inventory config ---- //
void HUD::InitInventoryFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("inventory")) return;
	const rapidjson::Value& inv = uiJson["inventory"];

	if (inv.HasMember("active"))
	{
		inventory.active = inv["active"].GetBool();
	}
	if (inv.HasMember("x"))
	{
		inventory.offsetX = inv["x"].GetFloat();
	}
	if (inv.HasMember("y"))
	{
		inventory.offsetY = inv["y"].GetFloat();
	}
	if (inv.HasMember("w"))
	{
		inventory.width = inv["w"].GetFloat();
	}
	if (inv.HasMember("h"))
	{
		inventory.height = inv["h"].GetFloat();
	}
	if (inv.HasMember("slotSize"))
	{
		inventory.slotSize = inv["slotSize"].GetFloat();
	}
	if (inv.HasMember("iconSize"))
	{
		inventory.iconSize = inv["iconSize"].GetFloat();
	}

	if (inv.HasMember("inventoryTexture"))
	{
		inventory.inventoryTexture = TextureManager::Get().LoadTexture(inv["inventoryTexture"].GetString());
	}
	if (inv.HasMember("slotTexture"))
	{
		inventory.slotTexture = TextureManager::Get().LoadTexture(inv["slotTexture"].GetString());
	}
	if (inv.HasMember("shieldIcon"))
	{
		inventory.shieldTexture = TextureManager::Get().LoadTexture(inv["shieldIcon"].GetString());
	}
	if (inv.HasMember("fullHpIcon"))
	{
		inventory.fullHpTexture = TextureManager::Get().LoadTexture(inv["fullHpIcon"].GetString());
	}
	if (inv.HasMember("dashIcon"))
	{
		inventory.dashTexture = TextureManager::Get().LoadTexture(inv["dashIcon"].GetString());
	}

	inventory.ready = true;
}

// ---- BuffBar config ---- //
void HUD::InitBuffBarFromConfig(const rapidjson::Value& uiJson)
{
	if (!uiJson.HasMember("buffBar")) return;
	const rapidjson::Value& bar = uiJson["buffBar"];

	if (bar.HasMember("active"))
	{
		buffBar.active = bar["active"].GetBool();
	}
	if (bar.HasMember("x"))
	{
		buffBar.x = bar["x"].GetFloat();
	}
	if (bar.HasMember("y"))
	{
		buffBar.y = bar["y"].GetFloat();
	}
	if (bar.HasMember("iconSize"))
	{
		buffBar.iconSize = bar["iconSize"].GetFloat();
	}
	if (bar.HasMember("gap"))
	{
		buffBar.gap = bar["gap"].GetFloat();
	}
	if (bar.HasMember("badgeTexture"))
	{
		buffBar.badgeTexture = TextureManager::Get().LoadTexture(bar["badgeTexture"].GetString());
	}

}

// ------------------------------------------------------------------------
// ---- Update HUD ---- //
void HUD::Update(float dt, const Player& player, PlayerWeapon weapon)
{
	if (!hudActive)
	{
		return;
	}

	// Update button states (hover/click animations)
	UpdateButtonStates(dt, globalCam.x, globalCam.y);

	// Hearts
	if (hearts.active && hearts.heartsSheet)
	{
		const int state = ClampHeartsStateFromPlayer(player);
		ApplyHeartsState(state);
	}

	// Weapon switching
	if (weapon != weaponSwitch.lastWeapon)
	{
		if (weapon == PlayerWeapon::MELEE)
		{
			if (weaponSwitch.meleeSheet)
			{
				weaponSwitch.meleeSheet->Play("selected", true);
			}
			if (weaponSwitch.gunSheet)
			{
				weaponSwitch.gunSheet->Play("selected", true);
				weaponSwitch.gunSheet->Stop();
			}
		}
		else if (weapon == PlayerWeapon::GUN)
		{
			if (weaponSwitch.gunSheet)
			{
				weaponSwitch.gunSheet->Play("selected", true);
			}
			if (weaponSwitch.meleeSheet)
			{
				weaponSwitch.meleeSheet->Play("selected", true);
				weaponSwitch.meleeSheet->Stop();
			}
		}

		weaponSwitch.lastWeapon = weapon;
	}

	if (weaponSwitch.meleeSheet)
		weaponSwitch.meleeSheet->Update(dt);
	if (weaponSwitch.gunSheet)
		weaponSwitch.gunSheet->Update(dt);
	
	// Progress bar
	pbarPlayerY = player.pos.y;

	UpdateBuffBar(player);
}

// ---- Update BuffBar ---- //
void HUD::UpdateBuffBar(const Player& player)
{
	buffBar.slotCount = 0;

	// Shield indicator
	if (player.shieldActive)
	{
		BuffBarSlot& slot = buffBar.slots[buffBar.slotCount++];
		slot.type = BuffType::SHIELD;
		slot.timer = player.shieldTimer;
		slot.duration = 10.0f;
		slot.uses = 0;
	}

	// Dash indicator
	if (player.dashEnabled)
	{
		BuffBarSlot& slot = buffBar.slots[buffBar.slotCount++];
		slot.type = BuffType::DASH;
		slot.timer = 0.0f;
		slot.duration = 0.0f;
		slot.uses = player.dashCharges;
	}

}

// ---- Update Button States ---- //
void HUD::UpdateButtonStates(float dt, float camX, float camY)
{
	if (!hudActive)
		return;

	float mouseX, mouseY;
	MousePosition(camX, camY, mouseX, mouseY);

	// Update pause button state
	if (pauseButton.active && pauseButton.ready)
	{
		const float cx = camX + pauseButton.offsetX;
		const float cy = camY + pauseButton.offsetY;
		const float halfW = pauseButton.width * 0.5f;
		const float halfH = pauseButton.height * 0.5f;

		bool hovered = (mouseX >= cx - halfW && mouseX <= cx + halfW &&
						mouseY >= cy - halfH && mouseY <= cy + halfH);

		pauseButtonState.isHovered = hovered;
		if (hovered)
			pauseButtonState.hoverAlpha = AEClamp(pauseButtonState.hoverAlpha + dt / 0.15f, 0.0f, 1.0f);
		else
			pauseButtonState.hoverAlpha = AEClamp(pauseButtonState.hoverAlpha - dt / 0.15f, 0.0f, 1.0f);
	}

	// Update inventory slot states
	if (inventory.active && inventory.ready)
	{
		const float x = camX + inventory.offsetX;
		const float y = camY + inventory.offsetY;
		const float slotSize = inventory.slotSize;

		for (int i = 0; i < 3; i++)
		{
			const float slotX = x + (i - 1) * (slotSize + 4.0f);
			const float halfSize = slotSize * 0.5f;

			bool hovered = (mouseX >= slotX - halfSize && mouseX <= slotX + halfSize &&
							mouseY >= y - halfSize && mouseY <= y + halfSize);

			// Only show hover if slot has a buff
			inventorySlotStates[i].isHovered = (hovered && inventory.slots[i] != BuffType::NONE && inventory.counts[i] > 0);
			if (inventorySlotStates[i].isHovered)
				inventorySlotStates[i].hoverAlpha = AEClamp(inventorySlotStates[i].hoverAlpha + dt / 0.15f, 0.0f, 1.0f);
			else
				inventorySlotStates[i].hoverAlpha = AEClamp(inventorySlotStates[i].hoverAlpha - dt / 0.15f, 0.0f, 1.0f);
		}
	}

	// Update weapon slot states
	if (weaponSwitch.active && weaponSwitch.ready)
	{
		const float hx = camX + weaponSwitch.offsetX;
		const float hy = camY + weaponSwitch.offsetY;
		const float slotSize = weaponSwitch.slotSize;
		const float halfSize = slotSize * 0.5f;

		// Melee slot (left)
		const float meleeX = hx - slotSize;
		bool meleeHovered = (mouseX >= meleeX - halfSize && mouseX <= meleeX + halfSize &&
							 mouseY >= hy - halfSize && mouseY <= hy + halfSize);

		weaponSlotStates[0].isHovered = meleeHovered;
		if (meleeHovered)
			weaponSlotStates[0].hoverAlpha = AEClamp(weaponSlotStates[0].hoverAlpha + dt / 0.15f, 0.0f, 1.0f);
		else
			weaponSlotStates[0].hoverAlpha = AEClamp(weaponSlotStates[0].hoverAlpha - dt / 0.15f, 0.0f, 1.0f);

		// Gun slot (right)
		const float gunX = hx;
		bool gunHovered = (mouseX >= gunX - halfSize && mouseX <= gunX + halfSize &&
						  mouseY >= hy - halfSize && mouseY <= hy + halfSize);

		weaponSlotStates[1].isHovered = gunHovered;
		if (gunHovered)
			weaponSlotStates[1].hoverAlpha = AEClamp(weaponSlotStates[1].hoverAlpha + dt / 0.15f, 0.0f, 1.0f);
		else
			weaponSlotStates[1].hoverAlpha = AEClamp(weaponSlotStates[1].hoverAlpha - dt / 0.15f, 0.0f, 1.0f);
	}

	// Decrement press timers
	for (int i = 0; i < 3; i++)
	{
		if (inventorySlotStates[i].pressTimer > 0.0f)
		{
			inventorySlotStates[i].pressTimer -= dt;
			if (inventorySlotStates[i].pressTimer < 0.0f)
				inventorySlotStates[i].pressTimer = 0.0f;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (weaponSlotStates[i].pressTimer > 0.0f)
		{
			weaponSlotStates[i].pressTimer -= dt;
			if (weaponSlotStates[i].pressTimer < 0.0f)
				weaponSlotStates[i].pressTimer = 0.0f;
		}
	}

	if (pauseButtonState.pressTimer > 0.0f)
	{
		pauseButtonState.pressTimer -= dt;
		if (pauseButtonState.pressTimer < 0.0f)
			pauseButtonState.pressTimer = 0.0f;
	}
}

// ---- Update Press Timers (called during Draw to ensure animation plays even during pause) ---- //
void HUD::UpdatePressTimers(float dt)
{
	for (int i = 0; i < 3; i++)
	{
		if (inventorySlotStates[i].pressTimer > 0.0f)
		{
			inventorySlotStates[i].pressTimer -= dt;
			if (inventorySlotStates[i].pressTimer < 0.0f)
				inventorySlotStates[i].pressTimer = 0.0f;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (weaponSlotStates[i].pressTimer > 0.0f)
		{
			weaponSlotStates[i].pressTimer -= dt;
			if (weaponSlotStates[i].pressTimer < 0.0f)
				weaponSlotStates[i].pressTimer = 0.0f;
		}
	}

	if (pauseButtonState.pressTimer > 0.0f)
	{
		pauseButtonState.pressTimer -= dt;
		if (pauseButtonState.pressTimer < 0.0f)
			pauseButtonState.pressTimer = 0.0f;
	}
}

void HUD::TriggerButtonPress(UIButtonState& state)
{
	state.isPressed = true;
	state.pressTimer = 0.1f;  // 0.1 second press duration
}

void HUD::TriggerWeaponSlotPress(int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 2)
		TriggerButtonPress(weaponSlotStates[slotIndex]);
}

void HUD::TriggerInventorySlotPress(int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 3)
		TriggerButtonPress(inventorySlotStates[slotIndex]);
}

void HUD::TriggerPauseButtonPress()
{
	TriggerButtonPress(pauseButtonState);
}

// ------------------------------------------------------------------------
// ---- Draw HUD ---- //
void HUD::Draw(MeshManager& meshManager, float camX, float camY, PlayerWeapon weapon) const
{
	if (!hudActive)
	{
		return;
	}

	// Draw core HUD elements in all modes
	DrawHearts(camX, camY);
	DrawPauseButton(camX, camY);
	DrawWeaponSwitch(camX, camY, weapon);
	DrawInventory(camX, camY);
	DrawBuffBar(camX, camY);

	// Progress bar only in main game (not boss room)
	if (!EnvironmentManager::Get().IsBossRoomMode())
	{
		DrawProgressBar(meshManager, camX, camY);
	}
}

// ---- Draw Hearts ---- //
void HUD::DrawHearts(float camX, float camY) const
{
	if (hearts.active && hearts.heartsSheet)
	{
		const float x = camX + hearts.offsetX;
		const float y = camY + hearts.offsetY;

		MeshManager::Get().DrawSpriteSheet(*hearts.heartsSheet, x, y, hearts.width, hearts.height, 1.0f);
	}
}

// ---- Draw Weapon Switch ---- //
void HUD::DrawWeaponSwitch(float camX, float camY, PlayerWeapon weapon) const
{
	if (weaponSwitch.active && weaponSwitch.ready)
	{
		const float hx = camX + weaponSwitch.offsetX;
		const float hy = camY + weaponSwitch.offsetY;

		const float slotSize = weaponSwitch.slotSize;
		const float iconSize = weaponSwitch.iconSize;

		const float meleeX = hx - slotSize;
		const float gunX = hx;

		// Highlight selected weapon
		if (weapon == PlayerWeapon::MELEE)
		{
			MeshManager::Get().DrawCircle(
				meleeX, hy,
				slotSize,
				230, 206, 154
			);
		}
		if (weapon == PlayerWeapon::GUN)
		{
			MeshManager::Get().DrawCircle(
				gunX, hy,
				slotSize,
				230, 206, 154
			);
		}

		// Draw weapon slots
		if (weaponSwitch.slotTexture)
		{
			// Melee slot (index 0)
			float meleeScale = 1.0f;
			if (weaponSlotStates[0].isPressed && weaponSlotStates[0].pressTimer > 0.0f)
			{
				float pressProgress = 1.0f - (weaponSlotStates[0].pressTimer / 0.1f);
				meleeScale = 1.0f - (0.1f * AEClamp(pressProgress, 0.0f, 1.0f));
			}
			float scaledMeleeSize = slotSize * meleeScale;

			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.slotTexture,
				meleeX, hy, scaledMeleeSize, scaledMeleeSize
			);

			// Gun slot (index 1)
			float gunScale = 1.0f;
			if (weaponSlotStates[1].isPressed && weaponSlotStates[1].pressTimer > 0.0f)
			{
				float pressProgress = 1.0f - (weaponSlotStates[1].pressTimer / 0.1f);
				gunScale = 1.0f - (0.1f * AEClamp(pressProgress, 0.0f, 1.0f));
			}
			float scaledGunSize = slotSize * gunScale;

			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.slotTexture,
				gunX, hy, scaledGunSize, scaledGunSize
			);
		}

		// Melee selected
		if (weaponSwitch.meleeSheet)
		{
			MeshManager::Get().DrawSpriteSheet(
				*weaponSwitch.meleeSheet,
				meleeX, hy, iconSize, iconSize, 1.0f
			);
		}
		// Gun selected
		if (weaponSwitch.gunSheet)
		{
			MeshManager::Get().DrawSpriteSheet(
				*weaponSwitch.gunSheet,
				gunX, hy, iconSize, iconSize, 1.0f
			);
		}

	}

}

// ---- Draw Progress Bar ---- //
void HUD::DrawProgressBar(MeshManager& meshManager, float camX, float camY) const
{
	// ---- Progress Bar ---- //
	if (progressBar.active)
	{
		const float hx = camX + progressBar.offsetX;
		const float hy = camY + progressBar.offsetY;

		const float pbarWidth = progressBar.width;
		const float pbarHeight = progressBar.height;

		const float segmentHeight = (pbarHeight - 2.0f * progressBar.gap) / 3.0f;
		const float pbarBottom = hy - pbarHeight * 0.5f;

		// Tracker position
		float trackerY;

		// Split progress bar into segments
		const float y0 = progressBar.minY;			 // bottom of progress bar
		const float y1 = progressBar.segmentEndY[0]; // first segment (bottom): level 1
		const float y2 = progressBar.segmentEndY[1]; // second segment (middle): level 2
		const float y3 = progressBar.segmentEndY[2]; // third segment (top): level 3

		if (pbarPlayerY <= y1)
		{
			float u = SegmentProgress(pbarPlayerY, y0, y1);
			trackerY = pbarBottom + 0 * (segmentHeight + progressBar.gap) + u * segmentHeight;
		}
		else if (pbarPlayerY <= y2)
		{
			float u = SegmentProgress(pbarPlayerY, y1, y2);
			trackerY = pbarBottom + 1 * (segmentHeight + progressBar.gap) + u * segmentHeight;
		}
		else
		{
			float u = SegmentProgress(pbarPlayerY, y2, y3);
			trackerY = pbarBottom + 2 * (segmentHeight + progressBar.gap) + u * segmentHeight;
		}

		const float minTrackY = pbarBottom + progressBar.paddingY + progressBar.trackerRadius;
		const float maxTrackY = (pbarBottom + pbarHeight) - progressBar.paddingY - progressBar.trackerRadius;

		if (trackerY < minTrackY)
		{
			trackerY = minTrackY;
		}
		if (trackerY > maxTrackY)
		{
			trackerY = maxTrackY;
		}

		// Draw progress bar
		if (progressBar.pbarReady && progressBar.texture)
		{
			MeshManager::Get().DrawTexturedSquare(
				progressBar.texture, hx, hy, pbarWidth, pbarHeight
			);
		}

		// Draw tracker
		meshManager.DrawCircle(hx, trackerY, progressBar.trackerRadius * 2.0f,
			progressBar.trackerR, progressBar.trackerG, progressBar.trackerB);
	}
}

// ---- Draw Pause Button ---- //
void HUD::DrawPauseButton(float camX, float camY) const
{
	// ---- Pause Button ---- //
	if (pauseButton.active && pauseButton.ready && pauseButton.texture)
	{
		float btnX = camX + pauseButton.offsetX;
		float btnY = camY + pauseButton.offsetY;
		float btnWidth = pauseButton.width;
		float btnHeight = pauseButton.height;

		// Apply press scale transformation
		float scale = 1.0f;
		if (pauseButtonState.isPressed && pauseButtonState.pressTimer > 0.0f)
		{
			// Scale decreases from 1.0 to 0.9 during press
			float pressProgress = 1.0f - (pauseButtonState.pressTimer / 0.1f);
			scale = 1.0f - (0.1f * AEClamp(pressProgress, 0.0f, 1.0f));
		}

		float scaledWidth = btnWidth * scale;
		float scaledHeight = btnHeight * scale;

		// Draw pause button with press scale
		MeshManager::Get().DrawTexturedSquare(
			pauseButton.texture,
			btnX, btnY,
			scaledWidth, scaledHeight, 1.0f
		);
	}
}

// ---- Draw Inventory ---- //
void HUD::DrawInventory(float camX, float camY) const
{
	if (!inventory.active || !inventory.ready) return;

	const float x = camX + inventory.offsetX;
	const float y = camY + inventory.offsetY;
	const float slotSize = inventory.slotSize;
	const float iconSize = inventory.iconSize;

	// Draw inventory overlay
	if (inventory.inventoryTexture)
	{
		MeshManager::Get().DrawTexturedSquare(
			inventory.inventoryTexture, x, y, inventory.width, inventory.height, 0.9f
		);
	}

	// Draw inventory slots
	for (int i = 0; i < (int)inventory.slots.size(); i++)
	{
		const float slotX = x + (i - 1) * (slotSize + 4.0f); // -1 to center 3 slots around x

		// Apply press scale transformation
		float scale = 1.0f;
		if (inventorySlotStates[i].isPressed && inventorySlotStates[i].pressTimer > 0.0f)
		{
			// Scale decreases from 1.0 to 0.9 during press
			float pressProgress = 1.0f - (inventorySlotStates[i].pressTimer / 0.1f);
			scale = 1.0f - (0.1f * AEClamp(pressProgress, 0.0f, 1.0f));
		}

		float scaledSlotSize = slotSize * scale;

		if (inventory.slotTexture)
		{
			MeshManager::Get().DrawTexturedSquare(
				inventory.slotTexture, slotX, y, scaledSlotSize, scaledSlotSize, 0.9f
			);
		}

		// Draw buffs in slots
		AEGfxTexture* buffIcon = nullptr;
		switch (inventory.slots[i]) {
		case BuffType::SHIELD:
			buffIcon = inventory.shieldTexture;
			break;
		case BuffType::FULL_HP:
			buffIcon = inventory.fullHpTexture;
			break;
		case BuffType::DASH:
			buffIcon = inventory.dashTexture;
			break;
		default: break;
		}

		if (buffIcon)
		{
			MeshManager::Get().DrawTexturedSquare(
				buffIcon, slotX, y, iconSize, iconSize);
		}

		// Draw stack count at bottom-right corner (if more than 1)
		if (inventory.counts[i] > 1)
		{
			char countStr[4];
			snprintf(countStr, sizeof(countStr), "%d", inventory.counts[i]);

			const float windowWidth = (float)AEGfxGetWindowWidth();
			const float windowHeight = (float)AEGfxGetWindowHeight();

			// Draw badge for text
			const float badgeSize = slotSize * 0.45f;
			const float badgeX = slotX + (slotSize * 0.5f) - (badgeSize * 0.5f);
			const float badgeY = y - (slotSize * 0.5f) + (badgeSize * 0.5f);

			if (buffBar.badgeTexture)
			{
				MeshManager::Get().DrawTexturedSquare(
					buffBar.badgeTexture, badgeX, badgeY,
					badgeSize, badgeSize, 0.9f
				);
			}
			else
			{
				MeshManager::Get().DrawSquare(
					badgeX, badgeY, badgeSize, badgeSize,
					0, 0, 0, 0.9f
				);
			}

		const float screenX = (badgeX - camX) / (windowWidth * 0.5f) - 0.006f;
		const float screenY = (badgeY - camY) / (windowHeight * 0.5f) - 0.012f;

		FontManager::Get().Print(FontManager::Get().GetSmallFont(), countStr, screenX, screenY, 0.6f, 1.0f, 0.95f, 0.2f, 1.0f);
	}
}
}

// ---- Draw HUD ---- //
void HUD::DrawBuffBar(float camX, float camY) const
{
	if (!buffBar.active || buffBar.slotCount == 0)
	{
		return;
	}

	const float iconSize = buffBar.iconSize;
	const float gap = buffBar.gap;
	const float windowWidth = (float)AEGfxGetWindowWidth();
	const float windowHeight = (float)AEGfxGetWindowHeight();

	const float totalWidth = buffBar.slotCount * iconSize + (buffBar.slotCount - 1) * gap;
	const float startX = camX + buffBar.x - totalWidth * 0.5f + iconSize * 0.5f;
	const float y = camY + buffBar.y;

	for (int i = 0; i < buffBar.slotCount; i++)
	{
		const BuffBarSlot& slot = buffBar.slots[i];
		const float x = startX + i * (iconSize + gap);

		// Draw slot
		if (inventory.slotTexture)
		{
			MeshManager::Get().DrawTexturedSquare(
				inventory.slotTexture, x, y, 
				iconSize + 5.0f, iconSize + 5.0f, 1.0f
			);
		}

		// Draw buff icons
		AEGfxTexture* icon = nullptr;
		switch (slot.type)
		{
		case BuffType::SHIELD:
			icon = inventory.shieldTexture;
			break;
		case BuffType::FULL_HP:
			icon = inventory.fullHpTexture;
			break;
		case BuffType::DASH:
			icon = inventory.dashTexture;
			break;
		default:
			break;
		}

		if (icon)
		{
			MeshManager::Get().DrawTexturedSquare(icon, x, y, iconSize, iconSize);
		}

		// Draw vertical depletion overlay
		if (slot.duration > 0.0f && slot.timer >= 0.0f)
		{
			float ratio = slot.timer / slot.duration;
			float overlayHeight = iconSize * (1.0f - ratio);
			float overlayY = (y + iconSize * 0.5f) - overlayHeight * 0.5f;

			if (overlayHeight > 0.0f)
			{
				MeshManager::Get().DrawSquare(
					x, overlayY, iconSize, overlayHeight, 0, 0, 0, 0.6f
				);
			}

		}

		// Draw timer or remaining buff uses
		char label[8];
		if (slot.duration > 0.0f)
		{
			snprintf(label, sizeof(label), "%d", (int)slot.timer + 1);
		}
		else
		{
			snprintf(label, sizeof(label), "%d", slot.uses);
		}

		// Draw badge for text
		const float badgeSize = iconSize * 0.45f;
		const float badgeX = x + (iconSize * 0.5f) - (badgeSize * 0.5f);
		const float badgeY = y - (iconSize * 0.5f) + (badgeSize * 0.5f);

		if (buffBar.badgeTexture)
		{
			MeshManager::Get().DrawTexturedSquare(
				buffBar.badgeTexture, badgeX, badgeY,
				badgeSize, badgeSize, 0.9f
			);
		}
		else
		{
			MeshManager::Get().DrawSquare(
				badgeX, badgeY, badgeSize, badgeSize,
				0, 0, 0, 0.9f
			);
		}

	const float screenX = (badgeX - camX) / (windowWidth * 0.5f) - 0.006f;
	const float screenY = (badgeY - camY) / (windowHeight * 0.5f) - 0.012f;

	FontManager::Get().Print(FontManager::Get().GetSmallFont(), label, screenX, screenY, 0.6f, 1.0f, 0.95f, 0.2f, 1.0f);
}

}

bool HUD::IsPauseButtonClicked(float camX, float camY) const
{
	if (!hudActive || !pauseButton.active || !pauseButton.ready || !pauseButton.texture)
		return false;

	if (!AEInputCheckTriggered(AEVK_LBUTTON))
		return false;

	float mouseX, mouseY;
	MousePosition(camX, camY, mouseX, mouseY);

	// Pause button center
	const float cx = camX + pauseButton.offsetX;
	const float cy = camY + pauseButton.offsetY;

	const float halfW = pauseButton.width * 0.5f;
	const float halfH = pauseButton.height * 0.5f;

	const float left = cx - halfW;
	const float right = cx + halfW;
	const float bottom = cy - halfH;
	const float top = cy + halfH;

	return (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top);
}

int HUD::IsInventorySlotClicked(float camX, float camY) const
{
	if (!hudActive || !inventory.active || !inventory.ready)
		return -1;

	if (!AEInputCheckTriggered(AEVK_LBUTTON))
		return -1;

	float mouseX, mouseY;
	MousePosition(camX, camY, mouseX, mouseY);

	const float x = camX + inventory.offsetX;
	const float y = camY + inventory.offsetY;
	const float slotSize = inventory.slotSize;

	// Check each slot (3 slots centered around x)
	for (int i = 0; i < 3; i++)
	{
		const float slotX = x + (i - 1) * (slotSize + 4.0f); // -1 to center 3 slots around x
		const float halfSize = slotSize * 0.5f;

		const float left = slotX - halfSize;
		const float right = slotX + halfSize;
		const float bottom = y - halfSize;
		const float top = y + halfSize;

		if (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top)
		{
			// Only return slot if it has a buff
			if (inventory.slots[i] != BuffType::NONE && inventory.counts[i] > 0)
				return i;
		}
	}

	return -1;
}

PlayerWeapon HUD::IsWeaponSlotClicked(float camX, float camY) const
{
	if (!hudActive || !weaponSwitch.active || !weaponSwitch.ready)
		return PlayerWeapon::NONE;

	if (!AEInputCheckTriggered(AEVK_LBUTTON))
		return PlayerWeapon::NONE;

	float mouseX, mouseY;
	MousePosition(camX, camY, mouseX, mouseY);

	const float hx = camX + weaponSwitch.offsetX;
	const float hy = camY + weaponSwitch.offsetY;
	const float slotSize = weaponSwitch.slotSize;

	// Melee slot (left)
	const float meleeX = hx - slotSize;
	const float halfSize = slotSize * 0.5f;

	float left = meleeX - halfSize;
	float right = meleeX + halfSize;
	float bottom = hy - halfSize;
	float top = hy + halfSize;

	if (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top)
		return PlayerWeapon::MELEE;

	// Gun slot (right)
	const float gunX = hx;

	left = gunX - halfSize;
	right = gunX + halfSize;
	bottom = hy - halfSize;
	top = hy + halfSize;

	if (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top)
		return PlayerWeapon::GUN;

	return PlayerWeapon::NONE;
}

bool HUD::IsAnyUIElementClicked(float camX, float camY) const
{
	if (!hudActive)
		return false;

	if (!AEInputCheckTriggered(AEVK_LBUTTON))
		return false;

	float mouseX, mouseY;
	MousePosition(camX, camY, mouseX, mouseY);

	// Check pause button
	if (pauseButton.active && pauseButton.ready)
	{
		const float cx = camX + pauseButton.offsetX;
		const float cy = camY + pauseButton.offsetY;
		const float halfW = pauseButton.width * 0.5f;
		const float halfH = pauseButton.height * 0.5f;

		if (mouseX >= cx - halfW && mouseX <= cx + halfW &&
			mouseY >= cy - halfH && mouseY <= cy + halfH)
			return true;
	}

	// Check inventory slots
	if (inventory.active && inventory.ready)
	{
		const float x = camX + inventory.offsetX;
		const float y = camY + inventory.offsetY;
		const float slotSize = inventory.slotSize;

		for (int i = 0; i < 3; i++)
		{
			const float slotX = x + (i - 1) * (slotSize + 4.0f);
			const float halfSize = slotSize * 0.5f;

			if (mouseX >= slotX - halfSize && mouseX <= slotX + halfSize &&
				mouseY >= y - halfSize && mouseY <= y + halfSize)
				return true;
		}
	}

	// Check weapon slots
	if (weaponSwitch.active && weaponSwitch.ready)
	{
		const float hx = camX + weaponSwitch.offsetX;
		const float hy = camY + weaponSwitch.offsetY;
		const float slotSize = weaponSwitch.slotSize;
		const float halfSize = slotSize * 0.5f;

		// Melee slot (left)
		const float meleeX = hx - slotSize;
		if (mouseX >= meleeX - halfSize && mouseX <= meleeX + halfSize &&
			mouseY >= hy - halfSize && mouseY <= hy + halfSize)
			return true;

		// Gun slot (right)
		const float gunX = hx;
		if (mouseX >= gunX - halfSize && mouseX <= gunX + halfSize &&
			mouseY >= hy - halfSize && mouseY <= hy + halfSize)
			return true;
	}

	return false;
}

void HUD::ClearInventory()
{
	for (int i = 0; i < (int)inventory.slots.size(); i++)
	{
		inventory.slots[i] = BuffType::NONE;
		inventory.counts[i] = 0;
	}
}

void HUD::AddBuffToInventory(BuffType buffType)
{
	for (int i = 0; i < (int)inventory.slots.size(); i++)
	{
		if (inventory.slots[i] == buffType)
		{
			inventory.counts[i]++;
			return;
		}
	}

	for (int i = 0; i < (int)inventory.slots.size(); i++)
	{
		if (inventory.slots[i] == BuffType::NONE)
		{
			inventory.slots[i] = buffType;
			inventory.counts[i] = 1;
			return;
		}
	}
}

void HUD::UseBuffFromInventory(Player& player, int slot)
{
	if (slot < 0 || slot >= (int)inventory.slots.size()) 
	{
		return;
	}
	if (inventory.slots[slot] == BuffType::NONE) 
	{
		return;
	}

	Buff b(inventory.slots[slot], 0.0f, 0.0f, 0.0f, 0.0f); // temp buff
	b.Activate(player);

	inventory.counts[slot]--;

	if (inventory.counts[slot] <= 0)
	{
		inventory.counts[slot] = 0;
		inventory.slots[slot] = BuffType::NONE;

		// shift remaining slots to the left when slot is empty
		for (int i = slot; i < (int)inventory.slots.size() - 1; i++)
		{
			inventory.slots[i] = inventory.slots[(size_t)i + 1];
			inventory.counts[i] = inventory.counts[(size_t)i + 1];
		}

		inventory.slots.back() = BuffType::NONE; // clear slot after shifting
		inventory.counts.back() = 0;
	}
}

int HUD::FindBuffSlot(BuffType buffType) const
{
	for (int i = 0; i < (int)inventory.slots.size(); i++)
	{
		if (inventory.slots[i] == buffType)
		{
			return i;
		}
	}

	return -1;
}
/* Start Header ************************************************************************/
/*!
\file	HUD.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par	kerwinjiajie.wong@digipen.edu
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
#include "SpriteSheet.h"
#include "Player.h"

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
	if (hpInt > 3)
	{
		hpInt = 3;
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

			hearts.heartsSheet->Play("hp3", true);
			hearts.heartsSheet->Stop();
			hearts.lastHeartsState = 3;
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
	if (weapon.HasMember("meleeIcon"))
	{
		weaponSwitch.meleeIcon =
			TextureManager::Get().LoadTexture(weapon["meleeIcon"].GetString());
	}
	if (weapon.HasMember("gunIcon"))
	{
		weaponSwitch.gunIcon =
			TextureManager::Get().LoadTexture(weapon["gunIcon"].GetString());
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
}

void HUD::Update(float /*dt*/, const Player& player)
{
	if (!hudActive)
	{
		return;
	}

	pbarPlayerY = player.pos.y;

	if (hearts.active && hearts.heartsSheet)
	{
		const int state = ClampHeartsStateFromPlayer(player);
		ApplyHeartsState(state);
	}

}

// ------------------------------------------------------------------------
// ---- Draw HUD ---- //
void HUD::Draw(MeshManager& meshManager, float camX, float camY, PlayerWeapon weapon) const
{
	if (!hudActive)
	{
		return;
	}

	DrawHearts(camX, camY);
	DrawWeaponSwitch(camX, camY, weapon);
	DrawProgressBar(meshManager, camX, camY);
	DrawPauseButton(camX, camY);
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
			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.slotTexture, 
				meleeX, hy, slotSize, slotSize
			);

			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.slotTexture, 
				gunX, hy, slotSize, slotSize
			);
		}

		// Draw weapon icons
		if (weaponSwitch.meleeIcon)
		{
			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.meleeIcon,
				meleeX, hy, iconSize, iconSize
			);
		}

		if (weaponSwitch.gunIcon)
		{
			MeshManager::Get().DrawTexturedSquare(
				weaponSwitch.gunIcon,
				gunX, hy, iconSize, iconSize
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
		const float y1 = progressBar.segmentEndY[0]; // first segment (bottom) - level 1
		const float y2 = progressBar.segmentEndY[1]; // second segment (middle) - level 2
		const float y3 = progressBar.segmentEndY[2]; // third segment (top) - level 3

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
		MeshManager::Get().DrawTexturedSquare(
			pauseButton.texture,
			camX + pauseButton.offsetX, camY + pauseButton.offsetY,
			pauseButton.width, pauseButton.height, 1.0f
		);
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

	// AABB collision
	return (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top);
}
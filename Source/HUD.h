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
	void Draw(MeshManager& meshManager, float camX, float camY) const;
	bool IsActive() const { return active; }

private:
	// ---- Hearts UI ---- //
	std::unique_ptr<SpriteSheet> heartsSheet;
	bool active = true;
	bool heartsActive = true;
	float heartsOffsetX = 680.0f;
	float heartsOffsetY = 410.0f;
	float heartsWidth = 128.0f;
	float heartsHeight = 128.0f;
	int lastHeartsState = -1;

	static int ClampHeartsStateFromPlayer(const Player& player);
	void ApplyHeartsState(int state);

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

		std::unique_ptr<SpriteSheet> pbarSheet;
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
		std::unique_ptr<SpriteSheet> pauseSheet;
		bool ready = false;
	};

	PauseButton pauseButton;

private:
	static float ClampProgressBar(float v);
	static float SegmentProgress(float y, float a, float b);
	void InitProgressBarFromConfig(const rapidjson::Value& uiJson);
	void InitPauseButtonFromConfig(const rapidjson::Value& uiJson);
};
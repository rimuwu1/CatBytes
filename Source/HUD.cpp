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

// Helper functions for Hearts UI
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

void HUD::ApplyHeartsState(int state)
{
	if (!heartsSheet)
	{
		return;
	}
	if (state == lastHeartsState)
	{
		return;
	}

	lastHeartsState = state;

	switch (state)
	{
	case 3: heartsSheet->Play("hp3", true);
		break;
	case 2: heartsSheet->Play("hp2", true);
		break;
	case 1: heartsSheet->Play("hp1", true);
		break;
	default: heartsSheet->Play("hp0", true);
		break;
	}

	heartsSheet->Stop();
}

// Helper functions for Progress Bar UI
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

float HUD::SegmentProgress(float y, float a, float b)
{
	if (b <= a)
	{
		return 0.0f;
	}

	return ClampProgressBar((y - a) / (b - a));
}

int HUD::To255(float v01)
{
	v01 = ClampProgressBar(v01);
	return (int)(v01 * 255.0f + 0.5f);
}

HUD::Colour HUD::RGB255(int r, int g, int b, int a)
{
	return Colour{
		r / 255.0f,
		g / 255.0f,
		b / 255.0f,
		a / 255.0f
	};
}

// ProgressBar config
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
	progressBar.barColours =
	{
		// level 1: blue #BDE7FF
		RGB255(189, 231, 255),

		// level 2: dark blue #5777A5
		RGB255(87, 119, 165),

		// level 3: purple #681AA3
		RGB255(104, 26, 163),

		//// level 4: dark purple #2B0438
		//RGB255(43, 4, 56)
	};

	if (pbar.HasMember("colours") && pbar["colours"].IsArray())
	{
		const rapidjson::Value& clr = pbar["colours"];
		if (clr.Size() >= 3)
		{
			for (int i = 0; i < 3; i++)
			{
				const rapidjson::Value& c = clr[i];
				if (c.HasMember("r") && c.HasMember("g") && c.HasMember("b"))
				{
					progressBar.barColours[i] = RGB255
					(
						c["r"].GetInt(),
						c["g"].GetInt(),
						c["b"].GetInt()
					);
				}
			}
		}
	}

	if (pbar.HasMember("animations"))
	{
		const rapidjson::Value& anims = pbar["animations"];

		if (anims.HasMember("file") && anims.HasMember("rows") && anims.HasMember("cols"))
		{
			progressBar.pbarSheet.reset(new SpriteSheet(
				anims["file"].GetString(),
				(u32)anims["rows"].GetInt(),
				(u32)anims["cols"].GetInt()
			));

			progressBar.pbarSheet->AddClip("frame", 0, 0, 0.0f, true);
			progressBar.pbarSheet->Play("frame", true);
			progressBar.pbarSheet->Stop();

			progressBar.pbarReady = true;
		}

	}

}


// HUD
void HUD::InitFromConfig(const rapidjson::Value& configDoc)
{
	active = true;

	// Hearts UI
	heartsActive = true;
	heartsOffsetX = 650.0f;
	heartsOffsetY = 400.0f;
	heartsWidth = 150.0f;
	heartsHeight = 40.0f;

	heartsSheet.reset();
	lastHeartsState = -1;

	// Progress Bar UI
	progressBar = ProgressBar();
	progressBar.active = false;

	if (!configDoc.HasMember("ui"))
	{
		return;
	}
	const rapidjson::Value& uiJson = configDoc["ui"];

	if (!uiJson.HasMember("hearts"))
	{
		return;
	}
	const rapidjson::Value& heartsJson = uiJson["hearts"];

	if (heartsJson.HasMember("active"))
	{
		heartsActive = heartsJson["active"].GetBool();
	}

	if (heartsJson.HasMember("x"))
	{
		heartsOffsetX = heartsJson["x"].GetFloat();
	}
	if (heartsJson.HasMember("y"))
	{
		heartsOffsetY = heartsJson["y"].GetFloat();
	}
	if (heartsJson.HasMember("w"))
	{
		heartsWidth = heartsJson["w"].GetFloat();
	}
	if (heartsJson.HasMember("h"))
	{
		heartsHeight = heartsJson["h"].GetFloat();
	}

	if (heartsJson.HasMember("animations"))
	{
		const rapidjson::Value& anims = heartsJson["animations"];

		if (anims.HasMember("file") && anims.HasMember("rows") && anims.HasMember("cols"))
		{
			heartsSheet.reset(new SpriteSheet(
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
					heartsSheet->AddClip(
						c["name"].GetString(),
						(u32)c["start"].GetInt(),
						(u32)c["end"].GetInt(),
						c["speed"].GetFloat(),
						c["loop"].GetBool()
					);
				}
			}

			heartsSheet->Play("hp3", true);
			heartsSheet->Stop();
			lastHeartsState = 3;
		}
	}

	InitProgressBarFromConfig(uiJson);
}

void HUD::Update(float /*dt*/, const Player& player)
{
	if (!active)
	{
		return;
	}

	pbarPlayerY = player.pos.y;

	if (heartsActive && heartsSheet)
	{
		const int state = ClampHeartsStateFromPlayer(player);
		ApplyHeartsState(state);
	}

}

void HUD::Draw(MeshManager& meshManager, float camX, float camY) const
{
	if (!active)
	{
		return;
	}

	// Hearts UI
	if (heartsActive && heartsSheet)
	{
		const float x = camX + heartsOffsetX;
		const float y = camY + heartsOffsetY;

		MeshManager::Get().DrawSpriteSheet(*heartsSheet, x, y, heartsWidth, heartsHeight, 1.0f);
	}

	// Progress Bar UI
	if (progressBar.active)
	{
		const float hx = camX + progressBar.offsetX;
		const float hy = camY + progressBar.offsetY;

		const float pbarWidth = progressBar.width;
		const float pbarHeight = progressBar.height;

		const float segmentHeight = (pbarHeight - 2.0f * progressBar.gap) / 3.0f;
		const float pbarBottom = hy - pbarHeight * 0.5f;

		const float innerWidth = pbarWidth - 2.0f * progressBar.paddingX;
		const float innerHeight = segmentHeight - /*2.0f **/ progressBar.paddingY;

		for (int i = 0; i < 3; i++)
		{
			const float segmentBottom = pbarBottom + i * (segmentHeight + progressBar.gap);

			float drawHeight = innerHeight;
			if (i < 2)
			{
				drawHeight += progressBar.overlapY;
			}

			float centerY = segmentBottom + progressBar.paddingY + drawHeight * 0.5f;

			const Colour& c = progressBar.barColours[i];
			meshManager.DrawSquare(hx, centerY, innerWidth, drawHeight, To255(c.r), To255(c.g), To255(c.b));
		}

		const float y0 = progressBar.minY;
		const float y1 = progressBar.segmentEndY[0];
		const float y2 = progressBar.segmentEndY[1];
		const float y3 = progressBar.segmentEndY[2];

		float trackerY;

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

		meshManager.DrawCircle(hx, trackerY, progressBar.trackerRadius * 2.0f,
			progressBar.trackerR, progressBar.trackerG, progressBar.trackerB);

		if (progressBar.pbarReady && progressBar.pbarSheet)
		{
			MeshManager::Get().DrawSpriteSheet(
				*progressBar.pbarSheet, hx, hy, pbarWidth, pbarHeight, 1.0f
			);
		}

	}

}
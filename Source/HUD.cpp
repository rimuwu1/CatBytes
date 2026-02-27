#include "HUD.h"
#include "MeshManager.h"
#include "SpriteSheet.h"
#include "Player.h"

#include <rapidjson/document.h>

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

void HUD::InitFromConfig(const rapidjson::Value& level1Config)
{
	active = true;
	heartsActive = true;
	heartsOffsetX = 650.0f;
	heartsOffsetY = 400.0f;
	heartsWidth = 150.0f;
	heartsHeight = 40.0f;

	heartsSheet.reset();
	lastHeartsState = -1;

	if (!level1Config.HasMember("ui"))
	{
		return;
	}
	const rapidjson::Value& uiJson = level1Config["ui"];

	//InitHearts(uiJson);

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
}

void HUD::Update(float /*dt*/, const Player& player)
{
	if (!active)
	{
		return;
	}
	if (!heartsActive)
	{
		return;
	}
	if (!heartsSheet)
	{
		return;
	}

	const int state = ClampHeartsStateFromPlayer(player);
	ApplyHeartsState(state);
}

void HUD::Draw(MeshManager& meshManager, float camX, float camY) const
{
	if (!active)
	{
		return;
	}
	if (!heartsActive)
	{
		return;
	}
	if (!heartsSheet)
	{
		return;
	}

	const float x = camX + heartsOffsetX;
	const float y = camY + heartsOffsetY;

	MeshManager::Get().DrawSpriteSheet(*heartsSheet, x, y, heartsWidth, heartsHeight, 1.0f);
}
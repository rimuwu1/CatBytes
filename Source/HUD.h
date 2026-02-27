#pragma once

#include "AEEngine.h"
//#include "Minimap.h"
#include <memory>
#include <rapidjson/document.h>

class MeshManager;
class SpriteSheet;

struct Player;

class HUD
{
public:
	void InitFromConfig(const rapidjson::Value& level1Config);
	void Update(float /*dt*/, const Player& player);
	void Draw(MeshManager& meshManager, float camX, float camY) const;
	bool IsActive() const { return active; }

private:
	std::unique_ptr<SpriteSheet> heartsSheet;
	bool active = true;
	bool heartsActive = true;
	float heartsOffsetX = 650.0f;
	float heartsOffsetY = 400.0f;
	float heartsWidth = 150.0f;
	float heartsHeight = 40.0f;
	int lastHeartsState = -1;

	//// Hearts UI
	//void InitHearts(const rapidjson::Value& uiJson);
	//void UpdateHearts(const Player& player);
	//void DrawHearts(MeshManager& meshmanager, float camX, float camY);
	//void ApplyHeartsState(int state);
private:
	static int ClampHeartsStateFromPlayer(const Player& player);
	void ApplyHeartsState(int state);

	//Minimap minimap;
};
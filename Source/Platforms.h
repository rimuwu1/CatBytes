/* Start Header ************************************************************************/
/*!
\file       Platforms.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
			Joash Ng, joash.ng, 2502780
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
			joash.ng@digipen.edu
\date       January 26 2026
\brief		This file contains the structs & function declarations needed for the platforms.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include <vector>
#include "Player.h"

struct Platform {
	float x, y, w, h;
	mutable bool active;				// allow modification even if it's const
};


struct PlatformButton {
	float x, y, w, h;
	std::vector<int> platformIndices;						// use vector to allow multiple platforms toggle
	std::vector<int> wallIndices;							// indices into toggle walls vector
	mutable bool wasPressed = false;						// allow modification even if it's const
	mutable bool prevState = false;							// track switch's previous state (active/inactive)
	mutable bool spriteInitialized = false;					// track sprite initialization
	std::unique_ptr<SpriteSheet> buttonSprite;				// sprite for the button
	std::string btnPrompt = "Press E to toggle platforms";	// default prompt
	bool blocksWhenDoorLocked = false;                      // block interaction until the boss door is unlocked
};

struct PlatformObstacle {
	float x, y, w, h, r;
	bool isSpike;
	mutable bool active = true;
	mutable float timer = 0.0f;
	float spikeInterval = 5.0f;
	std::unique_ptr<SpriteSheet> sprite;
	mutable bool spriteInitialized = false;
	mutable bool prevActive = true;
	bool alwaysStatic = false;
};

struct PlatformLaser {

	float x1, x2, y1, y2, w;
	mutable bool laserActive;
	mutable bool laserToggle;
	mutable float timer = 0.0f;
	float laserInterval = 5.0f;
};

struct Checkpoint {
	float x, y, w, h, r;
};

enum class BeamDirection { Horizontal, Vertical };
enum class ComputerToggle { Beam, BossDoor };

struct PlatformComputer {
	float x, y, w, h;
	std::vector<int> laserIndices;
	mutable bool wasPressed = false;
	mutable bool prevState = false;
	mutable bool spriteInitialized = false;

	mutable bool beamActive = false;
	BeamDirection direction = BeamDirection::Horizontal;
	float beamX1, beamX2, beamY1, beamY2, beamW = 50.0f;
	float beamStartX, beamEndX, beamStartY, beamEndY;
	float indW = 50.0f, indH = 50.0f;

	mutable bool pendingCameraPan = false;
	mutable float transitionTimer = 0.0f;

	mutable bool indicatorsVisible = false;
	mutable bool beamVisible = false;
	mutable bool nowShowBeam = false;
	mutable bool pendingActivate = false;
	mutable float preActivateDelay = 0.0f;
	mutable float delayBeamActivate = 0.0f;
	mutable bool beamKilled = false;
	mutable float beamDuration = 0.0f;

	ComputerToggle compType = ComputerToggle::Beam;

	// if true, interacting with this pc unlocks the boss door and pans the camera down to it
	bool unlocksBossDoor = false;

	std::unique_ptr<SpriteSheet> computerSprite;
	std::unique_ptr<SpriteSheet> indicatorLeft;
	std::unique_ptr<SpriteSheet> indicatorRight;

	bool  hackAnimPlaying   = false;
	float hackAnimTimer     = 0.0f;
	float hackAnimDuration  = 1.5f; // how long the "hacking" anim plays before pan starts
	
	bool  bossDoorHackComplete = false;  // true when boss door hack finished (no pan, show success prompt + particles)

};


//void Platforms_Draw(const std::vector<Platform>& platforms, AEGfxTexture* leftTex, AEGfxTexture* midTex, AEGfxTexture* rightTex);

bool Platform_CollisionCheck(Player& player, float& previosY, const std::vector<Platform>& platforms);

//void PlatformsObstacle_Draw(const std::vector<PlatformObstacle>& obstacles);

bool CheckObstacleCollision(const Player& player, const std::vector<PlatformObstacle>& obstacle);

void Platforms_OffsetY(std::vector<Platform>& platforms, float offsetY);

void WallCollisionCheck(Player& player, const std::vector<Platform>& wallPlatforms);

//void CheckpointDraw(const std::vector<Checkpoint>& checkpoint, const Player& player);

void CheckpointCollisionCheck(const Player& player, const std::vector<Checkpoint>& checkpoint, bool& checkpointHit, bool& checkpointInRange);
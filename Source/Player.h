/* Start Header ************************************************************************/
/*!
\file Player.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Tse Xuan Qi Tristin, tse.x, 2503757
		Joash ng, joash.ng, 2502780
		Sim Hui Min, s.huimin, 2503506
\par    kerwinjiajie.wong@digipen.edu
	    tse.x@digipen.edu
		joash.ng@digipen.edu
		s.huimin@digipen.edu
\date January, 23, 2026
\brief This file contains the function declarations for the Player movements, physics,
		input handling, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include "PlayerBullet.h"
#include "SpriteSheet.h"
#include "rapidjson/document.h"
#include <vector>
#include <memory>

// ----------------------------------------------------------------------------
// Types of weapons the player can equip
// easy to extend later (gun, Sword, etc.)
// ----------------------------------------------------------------------------
enum class PlayerWeapon
{
	NONE = 0,//No weapon equipped(basic player)
	MELEE,//Basic melee weapon
	GUN //gun weapon
};

enum class SlashDirection {
	HORIZONTAL,
	UP,
	DOWN
};

struct Player
{
	bool facingRight = true; // player jpg direction (current asset faces right)

	AEVec2 pos{ 0.0f, 0.0f };
	AEVec2 vel{ 0.0f, 0.0f };
	float width = 0.0f;
	float height = 0.0f;
	int grounded = 0;

	float hp = 0.0f; // player health
	float hitTextTimer = 0.0f;//got hit

	//Weapon system (extendable)
	PlayerWeapon weapon = PlayerWeapon::NONE;//which weapon is equipped
	bool weaponEquipped = false;//quick toggle flag

	//Melee attack state
	bool isAttacking = false;//true while melee swing is active
	bool downSlashJumped = false;   // true if down?slash has already caused a jump this swing
	float attackTimer = 0.0f;//how long the attack lasts
	float meleeDamage = 0.0f;//damage dealt to enemies
	float meleeCooldownTimer = 0.0f;
	
	// player gun
	int maxBullets = 0;
	float bulletSpeed = 0.0f;
	float bulletDamage = 0.0f;
	float fireCooldown = 0.0f;
	float fireTimer = 0.0f;
	float bulletWidth = 0.0f;
	float bulletHeight = 0.0f;

	std::vector<PlayerBullet> bullets; // player gun

	// Sprite sheet for animations
	std::unique_ptr<SpriteSheet> spriteSheet;
	std::unique_ptr<SpriteSheet> slashSprite;
	std::unique_ptr<SpriteSheet> bulletSprite;

	// State tracking for animation logic
	bool wasAttacking = false;
	bool wasWalking = false;
	PlayerWeapon previousWeapon = PlayerWeapon::NONE;
	SlashDirection slashDirection = SlashDirection::HORIZONTAL;
	bool weaponSwitchTriggered = false;
	bool weaponSwitchInProgress = false;

	// tail idle animation
	int idleLoopCount = 0;
	int idleLoopsBeforeTail = 5;
	bool playingTailAnim = false;

	// hurt state
	bool isHurt = false;
	bool wasHurt = false;
	float hurtTimer = 0.0f;

	// player
	u32 lastFrame = 0;

	// spike pogo
	bool pogoJustPerformed = false;
	float pogoVelocity = 0.0f;
};

struct Enemy; // forward declaration
struct PlatformObstacle;

void Player_Init(Player& player, const rapidjson::Value& config);
void Player_Update(Player& player, float dt);
void Player_Draw(const Player& player);
void Player_Free(Player& player);
void Player_ApplyDamage(Player& player, float damage);
void Player_CheckBulletCollisions(Player& player, Enemy& enemy);

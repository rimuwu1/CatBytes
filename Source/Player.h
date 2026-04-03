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
#include "Buff.h"
#include "SpriteSheet.h"
#include "ParticleManager.h"
#include "rapidjson/document.h"
#include <vector>
#include <memory>
#include <string>

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
	float maxHP = 5.0f;
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

	// buffs
	bool shieldActive = false;
	float shieldTimer = 0.0f;

	EmitterHandle shieldEmitters[4]{
		INVALID_EMITTER, INVALID_EMITTER, INVALID_EMITTER, INVALID_EMITTER
	};

	bool dashEnabled = false;
	bool isDashing = false;
	float dashTimer = 0.0f;
	float dashCooldown = 0.0f;
	float dashSpeed = 800.0f;
	int dashCharges = 0;
	EmitterHandle dashEmitter{ INVALID_EMITTER }; // trail effect during dash

	static constexpr float DASH_DURATION = 0.3f;
	static constexpr float DASH_COOLDOWN = 0.5f;

	std::vector<Buff> buffs;

	// Sprite sheet for animations
	std::unique_ptr<SpriteSheet> spriteSheet;
	std::unique_ptr<SpriteSheet> slashSprite;
	std::unique_ptr<SpriteSheet> bulletSprite;
	std::unique_ptr<SpriteSheet> shieldSprite;

	// State tracking for animation logic
	bool wasAttacking = false;
	bool wasWalking = false;
	PlayerWeapon previousWeapon = PlayerWeapon::NONE;
	SlashDirection slashDirection = SlashDirection::HORIZONTAL;
	bool weaponSwitchTriggered = false;
	bool weaponSwitchInProgress = false;
	std::string _desiredClip;
	bool _forceRestart = false;

	// tail idle animation
	int idleLoopCount = 0;
	int idleLoopsBeforeTail = 5;
	bool playingTailAnim = false;

	// hurt state
	bool isHurt = false;
	bool wasHurt = false;
	float hurtTimer = 0.0f;

	// knockback state
	AEVec2 knockbackVel{ 0.0f, 0.0f };
	float knockbackTimer = 0.0f;
	float knockbackVelocity = 0.0f;
	float knockbackAirUp = 0.0f;

	// player
	u32 lastFrame = 0;

	// spike pogo
	bool pogoJustPerformed = false;
	float pogoVelocity = 0.0f;

	// overrides the normal "press e" hint for special cases; cleared each frame by collision pass
	std::string interactHintOverride;
};

struct Enemy; // forward declaration
struct PlatformObstacle;

// Jump input tracking - true when player presses jump, cleared after sound plays
// Defined in Player.cpp, used in Input.cpp
extern bool s_JumpPressedThisFrame;

void Player_Init(Player& player, const rapidjson::Value& config);
void Player_Update(Player& player, float dt);
void Player_Draw(const Player& player);
void Player_Free(Player& player);
void Player_ApplyDamage(Player& player, float damage);
void Player_CheckBulletCollisions(Player& player, Enemy& enemy);
void Player_ApplyKnockback(Player& player, float sourceX, float sourceY);
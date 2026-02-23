/* Start Header ************************************************************************/
/*!
\file Player.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Tse Xuan Qi Tristin, tse.x, 2503757
\par kerwinajijie.wong@digipen.edu
	 tse.x@digipen.edu
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

struct Player
{
	bool facingRight = true; // player jpg direction (current asset faces right)

	AEGfxVertexList* mesh = nullptr;// shared quad mesh
	AEGfxTexture* texture = nullptr;//default player texture

	AEGfxTexture* meleeTexture = nullptr;//melee equipped
	AEGfxTexture* meleeAttackTexture = nullptr; //melee attack frame
	AEGfxTexture* meleeWeaponTexture = nullptr; // separate melee weapon sprite

	AEGfxTexture* gunTexture = nullptr;//gun equipped
	AEGfxTexture* gunAttackTexture = nullptr;//gun attack frame

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
	float attackTimer = 0.0f;//how long the attack lasts
	float meleeDamage = 0.0f;//damage dealt to enemies
	
	// player gun
	int maxBullets = 0;
	float bulletSpeed = 0.0f;
	float bulletDamage = 0.0f;
	float fireCooldown = 0.0f;
	float fireTimer = 0.0f;
	float bulletWidth = 0.0f;
	float bulletHeight = 0.0f;

	std::vector<PlayerBullet> bullets; // player gun

	//melee weapon animation
	float meleeWeaponYOffset = 0.0f;//current vertical offset
	float meleeWeaponMaxDrop = 50.0f;// how far weapon goes down
	bool meleeHasHitThisSwing = false; //prevents multi-hit per attack

	float meleeWeaponRotation = 0.0f;// current rotation angle in degrees
	float meleeWeaponRotationSpeed = 720.0f;//degrees per second for swing
	// Sprite sheet for animations
	std::unique_ptr<SpriteSheet> spriteSheet;

	// State tracking for animation logic
	bool wasAttacking = false;
	bool wasWalking = false;
	PlayerWeapon previousWeapon = PlayerWeapon::NONE;
	bool weaponSwitchTriggered = false;
};

struct Enemy; // forward declaration

void Player_Init(Player& player, float startX, float startY);
void Player_Update(Player& player, float dt);
void Player_Draw(const Player& player);
void Player_Free(Player& player);
void Player_ApplyDamage(Player& player, float damage);
void Player_CheckBulletCollisions(Player& player, Enemy& enemy);

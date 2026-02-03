/* Start Header ************************************************************************/
/*!
\file Player.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Tse Xuan Qi Tristin, tse.x, 2503757
		Sim Hui Min, s.huimin, 2503506
\par kerwinajijie.wong@digipen.edu
	 tse.x@digipen.edu
	 s.huimin@digipen.edu
\date January, 23, 2026
\brief This file contains the function definitions for the Player movements, physics,
		input handling, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "Player.h"
#include  "enemy.h"
#include "PlayerBullet.h"
#include "Utils.h"
#include "Fonts.h"
#include "Level1.h"
#include <fstream>
#include "GameStateManager.h"//for mainmenu test
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include <iostream>
#include <vector>

extern rapidjson::Document level1Config;



static float LoadPlayerHP() {
	float hp = level1Config["level_1"]["player"]["hp"].GetFloat();
	return (hp > 0.0f) ? hp : 20.0f; 
}

//loads melee damage from file (Player melee attack)
static float LoadMeleeDamage()
{
	float dmg = level1Config["level_1"]["player"]["melee_damage"].GetFloat();
	return (dmg > 0.0f)? dmg:5.0f; //fallback to 5 ifnone
}

static AEGfxVertexList* playerMesh = nullptr;
static AEGfxTexture* playerTexture = nullptr;//no weapon

static AEGfxTexture* playerMeleeTexture = nullptr;//melee equipped
static AEGfxTexture* playerMeleeAttackTexture = nullptr;//melee attack

static AEGfxTexture* playerGunTexture = nullptr;// gun equipped
static AEGfxTexture* playerGunAttackTexture = nullptr; // gun firing
static std::vector<PlayerBullet> playerBullets; // player gun


void Player_Init(Player& player, float startX, float startY)
{
	player.facingRight = true; // current player asset faces right on load

	// player gun bullets
	player.maxBullets = level1Config["level_1"]["player"]["bullet"]["max_count"].GetInt(); // player gun limit
	player.fireTimer = 0.0f;
	playerBullets.clear();
	playerBullets.resize(player.maxBullets);

	for (auto& b : playerBullets)
	{
		PlayerBullet_Init(b, player); // pass the player object
	}

	player.pos.x = startX;
	player.pos.y = startY;
	player.vel.x = 0.0f;
	player.vel.y = 0.0f;
	player.width = 80.0f;
	player.height = 80.0f;
	player.grounded = 1;

	//load player hp
	player.hp = LoadPlayerHP();

	// Weapon state
	player.weapon = PlayerWeapon::NONE;
	player.weaponEquipped = true;

	//melee attack state
	player.isAttacking = false;
	player.attackTimer = 0.0f;
	player.meleeDamage = LoadMeleeDamage();

	//gun
	const auto& playerJson = level1Config["level_1"]["player"];
	const auto& bulletJson = playerJson["bullet"];

	player.bulletSpeed = bulletJson["speed"].GetFloat();
	player.bulletDamage = bulletJson["damage"].GetFloat();
	player.fireCooldown = bulletJson["cooldown"].GetFloat();
	player.bulletWidth = bulletJson["width"].GetFloat();
	player.bulletHeight = bulletJson["height"].GetFloat();

	for (auto& b : playerBullets)
	{
		PlayerBullet_Init(b, player);
		b.damage = player.bulletDamage; // assign damage from JSON
	}

}

void Player_Update(Player& player, float dt)
{
	const float GRAVITY = -1200.0f;

	// Gravity
	player.vel.y += GRAVITY * dt;


	// Integrate velocity to position
	player.pos.x += player.vel.x * dt;
	player.pos.y += player.vel.y * dt;

	//For hit text
	//if (player.hitTextTimer > 0.0f)
		//player.hitTextTimer -= dt;

		//count down melee attack timer
	if (player.isAttacking)
	{
		player.attackTimer -= dt;
		if (player.attackTimer <= 0.0f)
		{
			player.attackTimer = 0.0f;
			player.isAttacking = false;
		}
	}

	// player image left/right 
	if (player.vel.x > 0.0f)
		player.facingRight = true;
	else if (player.vel.x < 0.0f)
		player.facingRight = false;

	// player gun
	// decrease fire timer
	if (player.fireTimer > 0.0f)
		player.fireTimer -= dt;

	//changed tomake it GUN
	// only fire if the player has the weapon equipped
	if (player.weaponEquipped && player.weapon == PlayerWeapon::GUN)
	{
		if (AEInputCheckTriggered(AEVK_LBUTTON) && player.fireTimer <= 0.0f)
		{
			for (auto& b : playerBullets)
			{
				if (!b.active)
				{
					b.active = true;
					b.pos = player.pos;
					b.vel.x = (player.facingRight ? 1.0f : -1.0f) * player.bulletSpeed;
					b.vel.y = 0.0f;

					player.fireTimer = player.fireCooldown;
					break; // only fire one bullet per click
				}
			}
		}
	}


	// update player bullets
	for (auto& b : playerBullets)
	{
		PlayerBullet_Update(b, dt);
	}

	// melee weapon tilt/swing
	if (player.weapon == PlayerWeapon::MELEE && player.weaponEquipped)
	{
		const float maxRotation = 100.0f; //swing (number) of degrees
		float rotationSpeed = player.meleeWeaponRotationSpeed;

		if (player.isAttacking)
		{
			// swing from vertical to horizontal
			player.meleeWeaponRotation += rotationSpeed * dt;
			if (player.meleeWeaponRotation > maxRotation)
				player.meleeWeaponRotation = maxRotation;
		}
		else
		{
			// return to vertical
			player.meleeWeaponRotation -= rotationSpeed * dt;
			if (player.meleeWeaponRotation < 0.0f)
				player.meleeWeaponRotation = 0.0f;
			player.meleeHasHitThisSwing = false;
		}
	}

}

void Player_Draw(const Player& player)
{
	// flip when moving left (default image faces right)
	float scaleX = player.facingRight ? player.width : -player.width;

	/*util::DrawTexturedSquare(
		player.mesh,
		(player.isAttacking) ? player.meleeAttackTexture :
		(player.weapon == PlayerWeapon::MELEE ? player.meleeTexture
			: player.texture),
		player.pos.x,
		player.pos.y,
		scaleX,
		player.height,
		1.0f
	);*/

	//TODO, will try to make it work another time
	/*
	  //draw hit text above the player
	if (player.hitTextTimer > 0.0f && g_FontSmall != -1)
	{
		float textX = player.pos.x;//same X as player
		float textY = player.pos.y + player.height * 0.5f + 20.0f; //above player

		AEGfxPrint(g_FontSmall, "Hit!", textX, textY,
			1.0f, 1.0f, 1.0f, 0.0f, 0.0f); //red
	}
	*/

	// drawing player bullets
	for (const auto& b : playerBullets)
		PlayerBullet_Draw(b);

	AEGfxTexture* currentTexture = nullptr;

	switch (player.weapon)
	{
	case PlayerWeapon::NONE:
		currentTexture = player.texture;
		break;
	case PlayerWeapon::MELEE:
		currentTexture = (player.isAttacking) ? player.meleeAttackTexture : player.meleeTexture;
		break;
	case PlayerWeapon::GUN:
		currentTexture = (player.fireTimer > 0.0f)
			? player.gunAttackTexture
			: player.gunTexture;
		break;
	}

	util::DrawTexturedSquare(
		player.mesh,
		currentTexture,
		player.pos.x,
		player.pos.y,
		scaleX,
		player.height,
		1.0f
	);


	//melee weapon visual parameters

	//size of the weapon sprite in world units
	const float weaponWidth = 30.0f;
	const float weaponHeight = 80.0f;

	//rotation angle for the weapon
	//flip direction when player faces left/right
	float rotDeg = -player.meleeWeaponRotation;//can use + or - to rotate differently
	if (!player.facingRight)
		rotDeg = -rotDeg;


	//Draw melee weapon sprite beside player when equipped
		//draw melee weapon sprite (rotates around handle)

	if (player.weapon == PlayerWeapon::MELEE &&
		player.weaponEquipped &&
		player.meleeWeaponTexture)
	{
		//how far the weapon sits from the player body
		float weaponOffsetX = player.facingRight
			? player.width * 0.5f + 10.0f
			: -player.width * 0.5f - 10.0f;

		//final weapon position in world space
		float weaponX = player.pos.x + weaponOffsetX;
		float weaponY = player.pos.y;

		//draw weapon with rotation around its bottom-center
		util::DrawTexturedSquarePivot(
			player.mesh,
			player.meleeWeaponTexture,
			weaponX,
			weaponY,
			weaponWidth,
			weaponHeight,
			rotDeg,
			0.0f,//pivot X: center of mesh
			0.5f,// pivot Y(handle)
			1.0f
		);
	}
}

//Apply damage to the player
 void Player_ApplyDamage(Player& player, float damage)
{
	if (player.hp <= 0.0f)
		return;

	player.hp -= damage;

	//show hit text for 0.5 seconds
	//player.hitTextTimer = 0.5f;

	//TEMPORARY death condition
	if (player.hp <= 0.0f)
	{
		player.hp = 0.0f;
		//trigger death: return to main menu
		next = GS_MAINMENU;
	}
}

 void Player_CheckBulletCollisions(Enemy& enemy)
 {
	 printf("CheckBulletCollisions called, enemy alive=%d, hp=%.1f\n", enemy.isAlive, enemy.hitPoints);
	 for (auto& b : playerBullets)
	 {
		 if (!b.active) continue;

		 printf("  bullet active at (%.1f, %.1f), enemy at (%.1f, %.1f)\n",
			 b.pos.x, b.pos.y, enemy.pos.x, enemy.pos.y);

		 if (!enemy.isAlive) continue;

		 float halfW_b = b.width * 0.5f;
		 float halfH_b = b.height * 0.5f;
		 float halfW_e = enemy.width * 0.5f;
		 float halfH_e = enemy.height * 0.5f;

		 bool overlapX = fabs(b.pos.x - enemy.pos.x) < (halfW_b + halfW_e);
		 bool overlapY = fabs(b.pos.y - enemy.pos.y) < (halfH_b + halfH_e);

		 printf("  overlapX=%d overlapY=%d | distX=%.1f need<%.1f | distY=%.1f need<%.1f\n",
			 overlapX, overlapY,
			 fabs(b.pos.x - enemy.pos.x), (halfW_b + halfW_e),
			 fabs(b.pos.y - enemy.pos.y), (halfH_b + halfH_e));

		 if (overlapX && overlapY)
		 {
			 printf("  HIT! damage=%.1f\n", b.damage);
			 enemy.hitPoints -= b.damage;
			 b.active = false;

			 if (enemy.hitPoints <= 0.0f)
				 enemy.isAlive = 0;
		 }
	 }
 }

// ----------------------------------------------------------------------------
// Releases all dynamically allocated resources used by the player
// this includes the player's mesh and texture, which are shared static resources
// ----------------------------------------------------------------------------
void Player_Free()
{
	//only for gameplay related memory
	
	playerBullets.clear();
	playerBullets.shrink_to_fit();
	// free bullets
	PlayerBullet_Free();
}

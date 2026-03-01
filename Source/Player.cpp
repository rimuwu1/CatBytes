/* Start Header ************************************************************************/
/*!
\file Player.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Tse Xuan Qi Tristin, tse.x, 2503757
		Sim Hui Min, s.huimin, 2503506
		Joash Ng, joash.ng, 2502780
\par kerwinajijie.wong@digipen.edu
	 tse.x@digipen.edu
	 s.huimin@digipen.edu
	 joash.ng@digipen.edu
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
#include "MeshManager.h"
#include "Fonts.h"
#include <fstream>
#include "GameStateManager.h"//for mainmenu test
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "SpriteSheet.h"
#include "AudioManager.h"
#include "Audio.h"

static AEAudio s_GunAttackSound{};
static AEAudio s_MeleeAttackSound{};
static bool s_PlayerAudioLoaded = false;


void Player_Init(Player& player, const rapidjson::Value& config)
{
	player.facingRight = true; // current player asset faces right on load

	// player gun bullets
	player.maxBullets = config["bullet"]["max_count"].GetInt(); // player gun limit
	player.fireTimer = 0.0f;

	//playerBullets.clear();
	//playerBullets.resize(player.maxBullets);
	player.bullets.clear();
	player.bullets.resize(player.maxBullets);

	player.pos.x = config.HasMember("x") ? config["x"].GetFloat() : 0.0f;
	player.pos.y = config.HasMember("y") ? config["y"].GetFloat() : -300.0f;
	player.vel.x = 0.0f;
	player.vel.y = 0.0f;
	player.width = config.HasMember("width") ? config["width"].GetFloat() : 80.0f;
	player.height = config.HasMember("height") ? config["height"].GetFloat() : 80.0f;
	player.grounded = 1;

	//load player hp & dmg
	player.hp = config["hp"].GetFloat();
	player.meleeDamage = config["melee_damage"].GetFloat();

	// Weapon state
	player.weapon = PlayerWeapon::NONE;
	player.weaponEquipped = true;

	//melee attack state
	player.isAttacking = false;
	player.attackTimer = 0.0f;

	//gun
	const auto& playerJson = config;
	const auto& bulletJson = playerJson["bullet"];

	player.bulletSpeed = bulletJson["speed"].GetFloat();
	player.bulletDamage = bulletJson["damage"].GetFloat();
	player.fireCooldown = bulletJson["cooldown"].GetFloat();
	player.bulletWidth = bulletJson["width"].GetFloat();
	player.bulletHeight = bulletJson["height"].GetFloat();

	for (auto& b : player.bullets)
	{
		PlayerBullet_Init(b, player);
		b.damage = player.bulletDamage; // assign damage from JSON
	}

	// --- Dynamic SpriteSheet Loading ---
	if (playerJson.HasMember("animations")) {
		const auto& anims = playerJson["animations"];
		player.spriteSheet = std::make_unique<SpriteSheet>(
			anims["file"].GetString(),
			anims["rows"].GetInt(),
			anims["cols"].GetInt()
		);

		const auto& clips = anims["clips"];
		for (rapidjson::SizeType i = 0; i < clips.Size(); i++) {
			const auto& c = clips[i];
			player.spriteSheet->AddClip(
				c["name"].GetString(),
				c["start"].GetInt(),
				c["end"].GetInt(),
				c["duration"].GetFloat(),
				c["loop"].GetBool()
			);
		}
	}
	// Start with idle
	player.spriteSheet->Play("idle");

	player.wasAttacking = false;
	player.wasWalking = false;
	player.previousWeapon = PlayerWeapon::NONE;
	player.weaponSwitchTriggered = false;

	// --- slash sheet Loading ---
	if (playerJson.HasMember("slash")) {
		const auto& anims = playerJson["slash"];
		player.slashSprite = std::make_unique<SpriteSheet>(
			anims["file"].GetString(),
			anims["rows"].GetInt(),
			anims["cols"].GetInt()
		);

		const auto& clips = anims["clips"];
		for (rapidjson::SizeType i = 0; i < clips.Size(); i++) {
			const auto& c = clips[i];
			player.slashSprite->AddClip(
				c["name"].GetString(),
				c["start"].GetInt(),
				c["end"].GetInt(),
				c["duration"].GetFloat(),
				c["loop"].GetBool()
			);
		}
	}
	// play slash
	player.slashSprite->Play("slash", true);
	if (!s_PlayerAudioLoaded)
	{
		s_GunAttackSound = AudioManager::Get().LoadAudio(Audio::PLAYER_GUN_ATTACK, false);
		s_MeleeAttackSound = AudioManager::Get().LoadAudio(Audio::PLAYER_MELEE_ATTACK, false);
		s_PlayerAudioLoaded = true;
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
			for (auto& b : player.bullets)
			{
				if (!b.active)
				{
					b.active = true;
					b.pos = player.pos;
					b.vel.x = (player.facingRight ? 1.0f : -1.0f) * player.bulletSpeed;
					b.vel.y = 0.0f;

					player.fireTimer = player.fireCooldown;
					AudioManager::Get().PlayAudio(s_GunAttackSound, false);
					break; // only fire one bullet per click
				}
			}
		}
	}


	// update player bullets
	for (auto& b : player.bullets)
	{
		PlayerBullet_Update(b, dt);
	}

	// Melee attack input (only if weapon is MELEE)
	if (player.weapon == PlayerWeapon::MELEE && player.weaponEquipped)
	{
		if (AEInputCheckTriggered(AEVK_LBUTTON))
		{
			player.isAttacking = true;

			AudioManager::Get().PlayAudio(s_MeleeAttackSound, false);

			// handle slash direction based on held keys
			if (AEInputCheckCurr('W'))
				player.slashDirection = SlashDirection::UP;
			else if (AEInputCheckCurr('S'))
				player.slashDirection = SlashDirection::DOWN;
			else // A or D (handle with flipping)
				player.slashDirection = SlashDirection::HORIZONTAL;

			// Reset jump flag for a new down‑slash
			if (player.slashDirection == SlashDirection::DOWN)
				player.downSlashJumped = false;

			// Activate slash effect
			if (player.slashSprite) {
				player.slashSprite->Play("slash", true);
				// Set player's attack timer to match slash duration
				float totalTime = player.slashSprite->GetClipTotalDuration("slash");
				player.attackTimer = totalTime;
			}
		}
	}

	if (player.slashSprite) {
		player.slashSprite->Update(dt);
	}
	

	// --- Determine walking state ---
	bool isWalking = (fabs(player.vel.x) > 0.1f) && player.grounded;
	// (you can adjust threshold as needed)

	// --- Detect weapon switch ---
	if (player.weapon != player.previousWeapon) {
		player.weaponSwitchTriggered = true;
		player.previousWeapon = player.weapon;
	}

	// --- Decide desired clip ---
	std::string desiredClip;
	bool forceRestart = false;

	// Handle weapon switch animation
	if (player.weaponSwitchTriggered) {
		desiredClip = "weapon_switch";
		// Force restart only if we just entered this clip
		if (player.spriteSheet->GetCurrentClip() != "weapon_switch") {
			forceRestart = true;
		}
		// If the switch clip has finished playing, clear the trigger
		if (player.spriteSheet->GetCurrentClip() == "weapon_switch" && !player.spriteSheet->IsPlaying()) {
			player.weaponSwitchTriggered = false;
		}
	}
	else {
		// Normal state based on weapon and actions
		switch (player.weapon) {
		case PlayerWeapon::NONE:
			desiredClip = isWalking ? "walk" : "idle";
			break;
		case PlayerWeapon::MELEE:
			if (player.isAttacking) {
				desiredClip = "melee_attack";
				if (!player.wasAttacking) forceRestart = true;
			}
			else {
				desiredClip = isWalking ? "melee_walk" : "melee_idle";
			}
			break;
		case PlayerWeapon::GUN:
			if (player.fireTimer > 0.0f) {
				desiredClip = "gun_attack";
				// Restart when firing starts (fireTimer == fireCooldown)
				if (fabs(player.fireTimer - player.fireCooldown) < 0.001f) {
					forceRestart = true;
				}
			}
			else {
				desiredClip = isWalking ? "gun_walk" : "gun_idle";
			}
			break;
		}
	}

	// Play the clip and update animation
	if (player.spriteSheet) {
		player.spriteSheet->Play(desiredClip, forceRestart);
		player.spriteSheet->Update(dt);
	}

	// Update state trackers
	player.wasAttacking = player.isAttacking;
	player.wasWalking = isWalking;
}

void Player_Draw(const Player& player)
{
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
	for (const auto& b : player.bullets)
		PlayerBullet_Draw(b);

	// Draw player using sprite sheet
	if (player.spriteSheet) {
		float scaleX = player.facingRight ? -player.width : player.width;
		MeshManager::Get().DrawSpriteSheet(
			*player.spriteSheet,
			player.pos.x,
			player.pos.y,
			scaleX,
			player.height,
			1.0f
		);
	}
	


	//melee weapon visual parameters
// Draw slash effect if active
	if (player.isAttacking && player.slashSprite)
	{
		float slashX = player.pos.x;
		float slashY = player.pos.y;
		float rotation = 0.0f;
		float offset = 20.0f;

		switch (player.slashDirection) {
		case SlashDirection::HORIZONTAL:
			slashX += (player.facingRight ? player.width * 0.5f + offset : -player.width * 0.5f - offset);
			break;
		case SlashDirection::UP:
			slashY += player.height * 0.5f + offset;
			rotation = 90.0f;
			break;
		case SlashDirection::DOWN:
			slashY -= player.height * 0.5f + offset;
			rotation = -90.0f;
			break;
		}

		float slashWidth = player.width;
		float slashHeight = player.height;

		// For horizontal, flip facing left
		float scaleX = -slashWidth;
		if (player.slashDirection == SlashDirection::HORIZONTAL && !player.facingRight)
			scaleX = slashWidth;

		MeshManager::Get().DrawSpriteSheet(
			*player.slashSprite,
			slashX,
			slashY,
			scaleX,
			slashHeight,
			1.0f,           // opacity
			rotation 
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
		GameStateManager::Get().next = GS_MAINMENU;
	}
}

 void Player_CheckBulletCollisions(Player& player, Enemy& enemy)
 {
	 printf("CheckBulletCollisions called, enemy alive=%d, hp=%.1f\n", enemy.isAlive, enemy.hitPoints);
	 for (auto& b : player.bullets)
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
void Player_Free(Player& player)
{
	//only for gameplay related memory
	// free bullets
	//PlayerBullet_Free();

	//free bullet internal resources (if any)
	for (auto& b : player.bullets)
		PlayerBullet_Free(b);

	//release vector heap memory
	player.bullets.clear();
	player.bullets.shrink_to_fit();

}

/* Start Header ************************************************************************/
/*!
\file Player.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Tse Xuan Qi Tristin, tse.x, 2503757
		Sim Hui Min, s.huimin, 2503506
		Joash Ng, joash.ng, 2502780
\par kerwinjiajie.wong@digipen.edu
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
#include "DebugManager.h"
#include "PhysicsManager.h"
#include "Platforms.h"
#include "Winlose.h"
#include "Camera.h"
#include "ParticleManager.h"

static AEAudio s_GunAttackSound{};
static AEAudio s_MeleeAttackSound{};
static AEAudio s_JumpSound{};
static bool s_PlayerAudioLoaded = false;

static const float MELEE_COOLDOWN = 0.3f;

void Player_Init(Player& player, const rapidjson::Value& config)
{
	player.facingRight = true; // current player asset faces right on load

	// player gun bullets
	player.maxBullets = config["bullet"]["max_count"].GetInt(); // player gun limit
	player.fireTimer = 0.0f;

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
	player.maxHP = player.hp;
	player.meleeDamage = config["melee_damage"].GetFloat();

	// knockback
	player.knockbackVelocity = config.HasMember("knockback_velocity") ? config["knockback_velocity"].GetFloat() : 400.0f;
	player.knockbackAirUp = config.HasMember("knockback_air_up") ? config["knockback_air_up"].GetFloat() : 300.0f;
	player.knockbackTimer = 0.0f;
	player.knockbackVel = { 0.0f, 0.0f };

	// Weapon state
	player.weapon = config.HasMember("weapon")
		? static_cast<PlayerWeapon>(config["weapon"].GetInt())
		: PlayerWeapon::NONE;
	player.weaponEquipped = true;

	//melee attack state
	player.isAttacking = false;
	player.attackTimer = 0.0f;
	player.meleeCooldownTimer = 0.0f;

	//gun
	const auto& playerJson = config;
	const auto& bulletJson = playerJson["bullet"];

	player.bulletSpeed = bulletJson["speed"].GetFloat();
	player.bulletDamage = bulletJson["damage"].GetFloat();
	player.fireCooldown = bulletJson["cooldown"].GetFloat();
	player.bulletWidth = bulletJson["width"].GetFloat();
	player.bulletHeight = bulletJson["height"].GetFloat();

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

	// idle tail animation system (to switch between idle anim 1 and idle anim 2 every few loops of 1st anim)
	player.idleLoopCount = 0;
	player.idleLoopsBeforeTail = 5 + (rand() % 6); // random between 5-10
	player.playingTailAnim = false;

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
	
	//load player audios
	if (!s_PlayerAudioLoaded)
	{
		s_GunAttackSound = AudioManager::Get().GetAudio("player_gun_attack");
		s_MeleeAttackSound = AudioManager::Get().GetAudio("player_melee_attack");
		s_JumpSound = AudioManager::Get().GetAudio("jump");
		s_PlayerAudioLoaded = true;
	}

	// player bullet sheet
	if (playerJson.HasMember("bullet_animations")) {
		const auto& anims = playerJson["bullet_animations"];
		player.bulletSprite = std::make_unique<SpriteSheet>(
			anims["file"].GetString(),
			anims["rows"].GetInt(),
			anims["cols"].GetInt()
		);
		const auto& clips = anims["clips"];
		for (rapidjson::SizeType i = 0; i < clips.Size(); i++) {
			const auto& c = clips[i];
			player.bulletSprite->AddClip(
				c["name"].GetString(),
				c["start"].GetInt(),
				c["end"].GetInt(),
				c["duration"].GetFloat(),
				c["loop"].GetBool()
			);
		}
		player.bulletSprite->Play("fly");
	}

	// initialize bullets with sprite 
	for (auto& b : player.bullets) {
		PlayerBullet_Init(b, player);
		b.bulletSprite = std::make_unique<SpriteSheet>(*player.bulletSprite);
		b.bulletSprite->Play("fly", true);
	}

	// restore buffs from save
	player.buffs.clear();
	if (config.HasMember("buffs") && config["buffs"].IsArray()) {
		for (const auto& b : config["buffs"].GetArray()) {
			BuffType t = static_cast<BuffType>(b["type"].GetInt());
			player.buffs.push_back(Buff(t, 0.f, 0.f, 50.f, 50.f));
			player.buffs.back().active = true;
		}
	}

	// hurt state
	player.isHurt = false;
	player.hurtTimer = 0.0f;
	player.wasHurt = false;

	// animation state tracking
	player.lastFrame = 0;
	player.weaponSwitchInProgress = false;
	player.downSlashJumped = false;

	// spike pogo
	player.pogoJustPerformed = false;
	player.pogoVelocity = config.HasMember("pogo_velocity") ? config["pogo_velocity"].GetFloat() : 600.0f;

	// dash state
	player.dashEnabled = false;
	player.isDashing = false;
	player.dashTimer = 0.0f;
	player.dashCooldown = 0.0f;
}

void Player_Update(Player& player, float dt)
{
    PhysicsManager& physics = PhysicsManager::Get();

    static int s_prevGrounded = 1;

    if (AEInputCheckTriggered(AEVK_SPACE))
    {
        // play jump sound
        AudioManager::Get().PlayAudio(s_JumpSound, false);

        // Jump dust burst from player feet
        ParticleManager_Emit(player.pos.x, player.pos.y - player.height * 0.5f,
            10, 120.f, 200, 200, 200);
    }

	// Apply gravity (skipped while grounded so velocity does not accumulate)
	physics.ApplyGravity(player.vel.y, static_cast<bool>(player.grounded), dt);

	// Cap fall speed to terminal velocity
	physics.ClampFallSpeed(player.vel.y);

	// hurt state
	if (player.isHurt) {
		player.hurtTimer -= dt;
		if (player.hurtTimer <= 0.0f) {
			player.hurtTimer = 0.0f;
			player.isHurt = false;
		}
	}

	// shield timer
	if (player.shieldActive) {
		player.shieldTimer -= dt;
		if (player.shieldTimer <= 0.0f) {
			player.shieldTimer = 0.0f;
			player.shieldActive = false;
		}
	}

	// dash timer
	if (player.isDashing) {
		player.dashTimer -= dt;
		player.vel.x = (player.facingRight ? 1.0f : -1.0f) * player.dashSpeed;

		if (player.dashTimer <= 0.0f) {
			player.dashTimer = 0.0f;
			player.isDashing = false;
			player.dashCooldown = Player::DASH_COOLDOWN;
			player.vel.x = 0.0f;
		}
	}

	if (player.dashCooldown > 0.0f) player.dashCooldown -= dt;

	// Euler-integrate velocity -> position
	physics.Integrate(player.pos, player.vel, dt);

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
					b.bulletSprite->Play("fly", true);
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
		// Decrement cooldown timer
		if (player.meleeCooldownTimer > 0.0f)
			player.meleeCooldownTimer -= dt;

		if (AEInputCheckTriggered(AEVK_LBUTTON) && player.meleeCooldownTimer <= 0.0f)
		{
			player.isAttacking = true;
			player.meleeCooldownTimer = MELEE_COOLDOWN;   // start cooldown

			AudioManager::Get().PlayAudio(s_MeleeAttackSound, false);

			// handle slash direction based on held keys
			if (AEInputCheckCurr('W'))
				player.slashDirection = SlashDirection::UP;
			else if (AEInputCheckCurr('S') && !player.grounded) // only allow downslash mid-air
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


	// --- Determine walking and jumping state ---
	bool isWalking = (fabs(player.vel.x) > 0.1f) && player.grounded;
	// (you can adjust threshold as needed)
	bool isInAir = !player.grounded;

	// --- Detect weapon switch ---
	//if (player.weapon != player.previousWeapon) {
	//	player.weaponSwitchTriggered = true;
	//	//player.previousWeapon = player.weapon;
	//}
	if (!player.weaponSwitchInProgress && player.weapon != player.previousWeapon) {
		player.weaponSwitchTriggered = true;
		player.weaponSwitchInProgress = true;
	}

	//unpause to prevent being stuck on frame
	if (player.spriteSheet->IsPaused()) {
		player.spriteSheet->SetPaused(false);
	}

	// --- Decide desired clip ---
	std::string desiredClip = "";
	bool forceRestart = false;

	// handle weapon switch animation (reworked)
	if (player.weaponSwitchTriggered) {
		// if player jumps during weapon switch, cancel the switch animation
		if (isInAir && player.vel.y > 0) {
			player.weaponSwitchTriggered = false;
			player.weaponSwitchInProgress = false;
		}
		else {
			if (player.weapon == PlayerWeapon::NONE) {
				// switching to normal
				if (player.previousWeapon == PlayerWeapon::MELEE)
					desiredClip = "claw2normal";
				else if (player.previousWeapon == PlayerWeapon::GUN)
					desiredClip = "gun2normal";
			}
			else if (player.weapon == PlayerWeapon::MELEE) {
				// switching to melee
				if (player.previousWeapon == PlayerWeapon::NONE)
					desiredClip = "normal2claw";
				else if (player.previousWeapon == PlayerWeapon::GUN)
					desiredClip = "gun2claw";
			}
			else if (player.weapon == PlayerWeapon::GUN) {
				// switching to gun
				if (player.previousWeapon == PlayerWeapon::NONE)
					desiredClip = "normal2gun";
				else if (player.previousWeapon == PlayerWeapon::MELEE)
					desiredClip = "claw2gun";
			}

			// force restart only if we just entered this clip
			if (!desiredClip.empty() && player.spriteSheet->GetCurrentClip() != desiredClip) {
				forceRestart = true;
			}

			// if the switch clip has finished playing, clear the trigger and update previousWeapon
			if (player.weaponSwitchTriggered &&
				player.spriteSheet->GetCurrentClip() == desiredClip &&
				!player.spriteSheet->IsPlaying()) {
				player.weaponSwitchTriggered = false;
				player.weaponSwitchInProgress = false;
				player.previousWeapon = player.weapon;
			}
		}
	}

	// dash animation
	if (player.isDashing && desiredClip.empty()) {
		desiredClip = "dash";
		forceRestart = !player.isDashing;
	}
	// hurt animation 
	else if (player.isHurt && desiredClip.empty()) {
		desiredClip = "hurt";
		if (!player.wasHurt) forceRestart = true;
	}
	// gun attack override everything except hurt
	else if (player.weapon == PlayerWeapon::GUN && player.fireTimer > 0.0f && desiredClip.empty())
	{
		desiredClip = "gunShoot";

		// restart when firing starts
		if (fabs(player.fireTimer - player.fireCooldown) < 0.001f)
			forceRestart = true;

		player.playingTailAnim = false;
		player.idleLoopCount = 0;
	}
	// melee aerial attacks (up/down) animate while jumping
	else if (isInAir && player.weapon == PlayerWeapon::MELEE && player.isAttacking && desiredClip.empty()) {
		if (player.slashDirection == SlashDirection::UP)
			desiredClip = "clawAttackUP";
		else if (player.slashDirection == SlashDirection::DOWN)
			desiredClip = "clawAttackDOWN";
		else
			desiredClip = "clawAttack"; // horizontal aerial attack

		if (!player.wasAttacking) forceRestart = true;
	}
	// jump animation when in air
	else if (isInAir && !player.weaponSwitchTriggered) {
		// check if moving sideways (A or D held)
		bool movingSideways = AEInputCheckCurr('A') || AEInputCheckCurr('D');
		if (movingSideways) {
			desiredClip = "jumpSIDE";
			// if already playing jumpSIDE and reached the end, freeze it
			if (player.spriteSheet->GetCurrentClip() == "jumpSIDE") {
				u32 currentFrame = player.spriteSheet->GetCurrentFrame();
				// frame 17 is the last frame of jumpSIDE (start: 15, end: 17)
				if (currentFrame >= 17) {
					player.spriteSheet->SetPaused(true);
				}
				else {
					forceRestart = false;
				}
			}
			else {
				forceRestart = true;
			}
		}
		else {
			desiredClip = "jumpFRONT";
			// if already playing jumpFRONT and reached the end, freeze it
			if (player.spriteSheet->GetCurrentClip() == "jumpFRONT") {
				u32 currentFrame = player.spriteSheet->GetCurrentFrame();
				// frame 12 is the last frame of jumpFRONT (start: 10, end: 12)
				if (currentFrame >= 12) {
					forceRestart = false;
					player.spriteSheet->SetPaused(true);
				}
				else {
					forceRestart = false;
				}
			}
			else {
				forceRestart = true;
			}
		}
	}
	if (desiredClip.empty()) {
		// when landing, unpause the spritesheet
		if (player.spriteSheet->IsPaused()) {
			player.spriteSheet->SetPaused(false);
		}
		// normal state based on weapon and actions
		switch (player.weapon) {
		case PlayerWeapon::NONE:
			if (isWalking) {
				desiredClip = "walk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				// idle tail animation logic
				if (player.playingTailAnim) {
					desiredClip = "idle2";
					// check if tail animation finished (non-looping clip stops)
					if (player.spriteSheet->GetCurrentClip() == "idle2" &&
						!player.spriteSheet->IsPlaying()) {
						player.playingTailAnim = false;
						player.idleLoopCount = 0;
						player.idleLoopsBeforeTail = 5 + (rand() % 6); // new random count
						desiredClip = "idle";
						forceRestart = true;
					}
				}
				else {
					desiredClip = "idle";
					// track frame changes to detect loop completion
					u32 lastFrame = player.lastFrame;
					u32 currentFrame = player.spriteSheet->GetCurrentFrame();
					// if frame wrapped back to start (loop completed)
					if (currentFrame < lastFrame && player.spriteSheet->GetCurrentClip() == "idle") {
						player.idleLoopCount++;
						// trigger tail animation after random loops
						if (player.idleLoopCount >= player.idleLoopsBeforeTail) {
							player.playingTailAnim = true;
							desiredClip = "idle2";
							forceRestart = true;
						}
					}
					player.lastFrame = currentFrame;
				}
			}
			break;

		case PlayerWeapon::MELEE:
			// auto-end attack when animation finishes
			if (player.isAttacking && !player.spriteSheet->IsPlaying()) {
				player.isAttacking = false;
			}

			if (player.isAttacking) {
				// choose attack animation based on slash direction
				if (player.slashDirection == SlashDirection::UP)
					desiredClip = "clawAttackUP";
				else
					desiredClip = "clawAttack"; // horizontal
				if (!player.wasAttacking) forceRestart = true;
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else if (isWalking) {
				desiredClip = "clawWalk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				desiredClip = "clawIdle";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			break;

		case PlayerWeapon::GUN:
			if (player.fireTimer > 0.0f) {
				desiredClip = "gunShoot";

				if (fabs(player.fireTimer - player.fireCooldown) < 0.001f)
					forceRestart = true;

				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else if (isWalking) {
				desiredClip = "gunWalk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				desiredClip = "gunIdle";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}

			break;
		}
	}

	// Play the clip and update animation
	if (player.spriteSheet && !desiredClip.empty()) {
		player.spriteSheet->Play(desiredClip, forceRestart);
		player.spriteSheet->Update(dt);
	}

    // Landing dust — fires once when player touches ground
    if (!s_prevGrounded && player.grounded) {
        ParticleManager_Emit(player.pos.x, player.pos.y - player.height * 0.5f,
            12, 100.f, 220, 220, 220);
    }
    s_prevGrounded = player.grounded;

    // Update state trackers
    player.wasAttacking = player.isAttacking;
    player.wasWalking = isWalking;
    player.wasHurt = player.isHurt;
}

void Player_Draw(const Player& player)
{
	
	// drawing player bullets (moved to obj manager)
	/*for (const auto& b : player.bullets)
		PlayerBullet_Draw(b);*/

	// Draw player using sprite sheet
	if (player.spriteSheet) {
		float scaleX;

		// special case: gun shooting needs to be flipped opposite to facing direction (else player shoots in wrong direction..) 
		if (player.weapon == PlayerWeapon::GUN && player.fireTimer > 0.0f) {
			// flip opposite: if facing right, flip left (negative), if facing left, flip right (positive)
			scaleX = player.facingRight ? player.width : -player.width;
		}
		else {
			// normal flip based on facing direction
			scaleX = player.facingRight ? -player.width : player.width;
		}

		MeshManager::Get().DrawSpriteSheet(
			*player.spriteSheet,
			player.pos.x,
			player.pos.y,
			scaleX,
			player.height,
			1.0f
		);
	}

	// Draw shield indicator
	if (player.shieldActive)
	{
		const float margin = 8.0f; // between player & shield

		const float left = player.pos.x - player.width * 0.5f - margin;
		const float right = player.pos.x + player.width * 0.5f + margin;
		const float bottom = player.pos.y - player.height * 0.5f - margin;
		const float top = player.pos.y + player.height * 0.5f + margin;

		MeshManager::Get().DrawLine(left, bottom, right, bottom, 3.0f, 50, 100, 255, 0.8f); // bottom
		MeshManager::Get().DrawLine(left, top, right, top, 3.0f, 50, 100, 255, 0.8f);		// top
		MeshManager::Get().DrawLine(left, bottom, left, top, 3.0f, 50, 100, 255, 0.8f);		// left
		MeshManager::Get().DrawLine(right, bottom, right, top, 3.0f, 50, 100, 255, 0.8f);	// right
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
		float scaleX, scaleY;

		switch (player.slashDirection) {
		case SlashDirection::HORIZONTAL:
			// Horizontal slash: flip horizontally based on facing
			scaleX = player.facingRight ? -slashWidth : slashWidth;
			scaleY = slashHeight; // no vertical flip
			break;

		case SlashDirection::UP:
			// Up slash: flip vertically if facing right
			scaleX = -slashWidth; // flip horizontally for both directions to maintain correct orientation
			scaleY = player.facingRight ? -slashHeight : slashHeight;
			break;

		case SlashDirection::DOWN:
			// Down slash: flip vertically if facing left
			scaleX = -slashWidth; // flip horizontally for both directions to maintain correct orientation
			scaleY = player.facingRight ? slashHeight : -slashHeight;
			break;

		default:
			scaleX = slashWidth;
			scaleY = slashHeight;
			break;
		}

		MeshManager::Get().DrawSpriteSheet(
			*player.slashSprite,
			slashX,
			slashY,
			scaleX,
			scaleY,
			1.0f,           // opacity
			rotation
		);
	}
}

//Apply damage to the player
void Player_ApplyDamage(Player& player, float damage)
{
	if (player.hp <= 0.0f) return;
	if (player.isHurt) return; // i-frames active, ignore damage
	if (player.shieldActive) return;
	if (player.isDashing) return;
	if (!DebugManager::Get().IsGodModeActive()) {
		player.hp -= damage;

		// trigger hurt animation
		player.isHurt = true;
		player.hurtTimer = player.spriteSheet->GetClipTotalDuration("hurt");

		Camera_AddTrauma(0.6f);
		ParticleManager_Emit(player.pos.x, player.pos.y, 12, 250.f, 255, 50, 50);

		//show hit text for 0.5 seconds
		//player.hitTextTimer = 0.5f;

		//TEMPORARY death condition
		if (player.hp <= 0.0f)
		{
			player.hp = 0.0f;
			//trigger death: return to main menu
			textScreenMessage = "You Lose";
			GameStateManager::Get().next = GS_WINLOSE;
		}
	}
}

void Player_ApplyKnockback(Player& player, float sourceX, float sourceY)
{
	float dirX = player.pos.x - sourceX;
	float dirY = player.pos.y - sourceY;

	float length = sqrtf(dirX * dirX + dirY * dirY);
	if (length > 0.0f)
	{
		dirX /= length;
		dirY /= length;
	}

	player.knockbackVel.x = dirX * player.knockbackVelocity;
	player.knockbackVel.y = dirY * player.knockbackVelocity;

	if (player.grounded)
	{
		player.knockbackVel.y = 0.0f;
	}
	else
	{
		player.knockbackVel.y = player.knockbackAirUp;
	}

	player.knockbackTimer = player.hurtTimer;
	player.vel.x = player.knockbackVel.x;
	player.vel.y = player.knockbackVel.y;
}

void Player_CheckBulletCollisions(Player& player, Enemy& enemy)
{
	for (auto& b : player.bullets)
	{
		if (!b.active) continue;

		if (!enemy.isAlive) continue;

		float halfW_b = b.width * 0.5f;
		float halfH_b = b.height * 0.5f;
		float halfW_e = enemy.width * 0.5f;
		float halfH_e = enemy.height * 0.5f;

		bool overlapX = fabs(b.pos.x - enemy.pos.x) < (halfW_b + halfW_e);
		bool overlapY = fabs(b.pos.y - enemy.pos.y) < (halfH_b + halfH_e);

		if (overlapX && overlapY)
		{
			enemy.hitPoints -= b.damage;
			b.active = false;

			// Knockback direction: same as bullet velocity
			float knockbackDir = (b.vel.x > 0.0f) ? 1.0f : -1.0f;
			Enemy_OnHit(enemy, b.damage, knockbackDir);
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

	//free bullet internal resources (if any)
	for (auto& b : player.bullets)
		PlayerBullet_Free(b);

	//release vector heap memory
	player.bullets.clear();
	player.bullets.shrink_to_fit();

}

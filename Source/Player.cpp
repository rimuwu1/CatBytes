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
#include "EnvironmentManager.h"

namespace {
    // Shield visual constants
    constexpr float SHIELD_WIDTH_OFFSET = 35.0f;
    constexpr float SHIELD_HEIGHT_OFFSET = 45.0f;
    
    // Block pushback force (from boss blocking player attack)
    constexpr float BLOCK_PUSHBACK_HORIZONTAL = 1400.0f;
    constexpr float BLOCK_PUSHBACK_VERTICAL = 200.0f;
}

static AEAudio s_GunAttackSound{};
static AEAudio s_MeleeAttackSound{};
static AEAudio s_JumpSound{};
static AEAudio s_WeaponSwitchClaws{};
static AEAudio s_WeaponSwitchGuns{};
static AEAudio s_PogoSound{};
static AEAudio s_PlayerDamageSound{};
static bool s_PlayerAudioLoaded = false;
static bool s_WasGrounded = true;
static bool s_WeaponSwitchSoundPlayed = false;
static bool s_PlayerDamageSoundPlayed = false;
bool s_JumpPressedThisFrame = false; // true when player presses jump, cleared after sound plays

static const float MELEE_COOLDOWN = 0.3f;
static int s_prevGrounded = 1;

namespace {
    void Player_HandleJumpInput(Player& player);
    void Player_HandlePogoSound(Player& player);
    void Player_HandleHurt(Player& player, float dt);
    bool Player_HandleKnockback(Player& player, float dt, PhysicsManager& physics);
    void Player_HandleShield(Player& player, float dt);
    void Player_HandleDash(Player& player, float dt);
    void Player_HandleCombat(Player& player, float dt, bool uiClicked);
    void Player_UpdateWeaponSwitchAnimation(Player& player, float dt);
    void Player_UpdateMovementAnimation(Player& player, float dt, bool isWalking, bool isInAir);
    void Player_HandleLandingDust(Player& player);
    void Player_UpdateStateTrackers(Player& player, bool isWalking);
}

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
	/*player.maxHP = player.hp;*/
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
		s_WeaponSwitchClaws = AudioManager::Get().GetAudio("weapon_switch_claws");
		s_WeaponSwitchGuns = AudioManager::Get().GetAudio("weapon_switch_guns");
		s_PogoSound = AudioManager::Get().GetAudio("pogo");
		s_PlayerDamageSound = AudioManager::Get().GetAudio("player_damage");
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

	// shield sheet
	if (playerJson.HasMember("shield_animations")) {
		const auto& anims = playerJson["shield_animations"];
		player.shieldSprite = std::make_unique<SpriteSheet>(
			anims["file"].GetString(),
			anims["rows"].GetInt(),
			anims["cols"].GetInt()
		);

		const auto& clips = anims["clips"];
		for (rapidjson::SizeType i = 0; i < clips.Size(); i++) {
			const auto& c = clips[i];
			player.shieldSprite->AddClip(
				c["name"].GetString(),
				c["start"].GetInt(),
				c["end"].GetInt(),
				c["duration"].GetFloat(),
				c["loop"].GetBool()
			);
		}
		// looping animation
		player.shieldSprite->Play("loop", true);
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

	// buff states
	player.shieldActive = false;
	player.dashEnabled = false;
	player.isDashing = false;
	player.dashTimer = 0.0f;
	player.dashCooldown = 0.0f;
	player.dashCharges = 0;

	// shield particles
	for (int i = 0; i < 4; ++i) {
		player.shieldEmitters[i] = INVALID_EMITTER;
	}

	// dash trail particles
	player.dashEmitter = INVALID_EMITTER;
}

void Player_Update(Player& player, float dt)
{
	PhysicsManager& physics = PhysicsManager::Get();

	Player_HandleJumpInput(player);
	Player_HandlePogoSound(player);

	// Apply gravity (skipped while grounded or dashing so velocity does not accumulate)
	if (!player.isDashing) {
		physics.ApplyGravity(player.vel.y, static_cast<bool>(player.grounded), dt);
		physics.ClampFallSpeed(player.vel.y);
	}

	Player_HandleHurt(player, dt);

	if (Player_HandleKnockback(player, dt, physics))
		return;

	Player_HandleShield(player, dt);
	Player_HandleDash(player, dt);

	HUD& hud = EnvironmentManager::Get().GetHUD();
	bool uiClicked = hud.IsAnyUIElementClicked(globalCam.x, globalCam.y);
	physics.Integrate(player.pos, player.vel, dt);
	Player_HandleCombat(player, dt, uiClicked);

	bool isWalking = (fabs(player.vel.x) > 0.1f) && player.grounded;
	bool isInAir = !player.grounded;

	Player_UpdateWeaponSwitchAnimation(player, dt);
	Player_UpdateMovementAnimation(player, dt, isWalking, isInAir);
	Player_HandleLandingDust(player);
	Player_UpdateStateTrackers(player, isWalking);
}

namespace {

void Player_HandleJumpInput(Player& player)
{
	// Only play jump sound if player actually pressed jump (not just walking off platforms)
	if (s_JumpPressedThisFrame)
	{
		AudioManager::Get().PlayAudio(s_JumpSound, false);
		s_JumpPressedThisFrame = false;
	}
}

void Player_HandlePogoSound(Player& player)
{
	static bool s_PogoPlayed = false;
	if (player.pogoJustPerformed && !s_PogoPlayed)
	{
		AudioManager::Get().PlayAudio(s_PogoSound, false);
		s_PogoPlayed = true;
	}
	if (!player.pogoJustPerformed)
	{
		s_PogoPlayed = false;
	}
}

void Player_HandleHurt(Player& player, float dt)
{
	// hurt state
	if (player.isHurt) {
		player.hurtTimer -= dt;
		if (player.hurtTimer <= 0.0f) {
			player.hurtTimer = 0.0f;
			player.isHurt = false;

			// reset sound flag when hurt ends
			s_PlayerDamageSoundPlayed = false;
		}
	}
}

bool Player_HandleKnockback(Player& player, float dt, PhysicsManager& physics)
{
	// knockback — owns the update loop for its duration, skips all input
	if (player.knockbackTimer > 0.0f)
	{
		player.knockbackTimer -= dt;
		if (player.knockbackTimer < 0.0f) player.knockbackTimer = 0.0f;

		if (!player.isDashing) {
			physics.ApplyGravity(player.vel.y, static_cast<bool>(player.grounded), dt);
			physics.ClampFallSpeed(player.vel.y);
		}
		physics.Integrate(player.pos, player.vel, dt);

		if (player.spriteSheet) {
			if (player.spriteSheet->GetCurrentClip() != "hurt")
				player.spriteSheet->Play("hurt", true);
			player.spriteSheet->Update(dt);
		}
		if (player.slashSprite) player.slashSprite->Update(dt);
		for (auto& b : player.bullets) PlayerBullet_Update(b, dt);

		player.wasHurt = player.isHurt;
		s_prevGrounded = player.grounded;
		return true;
	}
	return false;
}

void Player_HandleShield(Player& player, float dt)
{
	// active shield
	if (player.shieldActive) {

		// timer
		player.shieldTimer -= dt;
		if (player.shieldTimer <= 0.0f) {
			player.shieldTimer = 0.0f;
			player.shieldActive = false;
		}

		// animation
		if (player.shieldActive && player.shieldSprite) {
			if (player.shieldSprite->GetCurrentClip() != "loop")
				player.shieldSprite->Play("loop", true);

			player.shieldSprite->Update(dt);
		}

		// particles
		const float margin = 8.0f;
		const float offset = 4.0f;

		const float left = player.pos.x - player.width * 0.5f - margin;
		const float right = player.pos.x + player.width * 0.5f + margin;
		const float bottom = player.pos.y - player.height * 0.5f - margin;
		const float top = player.pos.y + player.height * 0.5f + margin;

		// random particle spawn along shield edges
		float topX = left + ((float)rand() / RAND_MAX) * (right - left);
		float bottomX = left + ((float)rand() / RAND_MAX) * (right - left);
		float leftY = bottom + ((float)rand() / RAND_MAX) * (top - bottom);
		float rightY = bottom + ((float)rand() / RAND_MAX) * (top - bottom);

		const float positions[4][2] = {
			{ topX, top - offset },			// top
			{ bottomX, bottom + offset },	// bottom
			{ left + offset, leftY },		// left
			{ right - offset, rightY }		// right
		};

		for (int i = 0; i < 4; ++i) {
			if (player.shieldEmitters[i] == INVALID_EMITTER) {
				player.shieldEmitters[i] = ParticleManager_EmitterStart(
					positions[i][0], positions[i][1],
					20,
					18.0f,
					80, 160, 255,
					0.18f, 0.28f,
					2.0f, 4.0f
				);
			}
			else {
				ParticleManager_EmitterMove(
					player.shieldEmitters[i],
					positions[i][0], positions[i][1]
				);
			}
		}
	}
	else {
		for (int i = 0; i < 4; ++i) {
			if (player.shieldEmitters[i] != INVALID_EMITTER) {
				ParticleManager_EmitterStop(player.shieldEmitters[i]);
				player.shieldEmitters[i] = INVALID_EMITTER;
			}
		}
	}
}

void Player_HandleDash(Player& player, float dt)
{
	// dash timer
	if (player.isDashing) {
		player.dashTimer -= dt;
		player.vel.x = (player.facingRight ? 1.0f : -1.0f) * player.dashSpeed;
		player.vel.y = 0.0f;  // maintain horizontal dash, no vertical movement

		// Start dash trail emitter if not already running
		if (player.dashEmitter == INVALID_EMITTER) {
			player.dashEmitter = ParticleManager_EmitterStart(
				player.pos.x, player.pos.y,
				60,                         // particles per second
				100.0f,                     // speed
				255, 255, 255,             // white trail
				0.1f, 0.2f,                // short life for fast-moving trail
				3.0f, 6.0f                 // small particles
			);
		} else {
			// Move emitter to player position
			ParticleManager_EmitterMove(player.dashEmitter, player.pos.x, player.pos.y);
		}

		if (player.dashTimer <= 0.0f) {
			player.dashTimer = 0.0f;
			player.isDashing = false;
			player.dashCooldown = Player::DASH_COOLDOWN;
			player.vel.x = 0.0f;
			// Stop dash trail emitter
			if (player.dashEmitter != INVALID_EMITTER) {
				ParticleManager_EmitterStop(player.dashEmitter);
				player.dashEmitter = INVALID_EMITTER;
			}
		}
	}

	if (player.dashCooldown > 0.0f) player.dashCooldown -= dt;
}

void Player_HandleCombat(Player& player, float dt, bool uiClicked)
{
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
		if (AEInputCheckTriggered(AEVK_LBUTTON) && player.fireTimer <= 0.0f && !uiClicked)
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

		if (AEInputCheckTriggered(AEVK_LBUTTON) && player.meleeCooldownTimer <= 0.0f && !uiClicked)
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
}

void Player_UpdateWeaponSwitchAnimation(Player& player, float dt)
{
	(void)dt;
	// --- Detect weapon switch ---
	//if (player.weapon != player.previousWeapon) {
	//	player.weaponSwitchTriggered = true;
	//	//player.previousWeapon = player.weapon;
	//}
	if (!player.weaponSwitchInProgress && player.weapon != player.previousWeapon) {
		player.weaponSwitchTriggered = true;
		player.weaponSwitchInProgress = true;

		if (!s_WeaponSwitchSoundPlayed)
		{
			switch (player.weapon)
			{
			case PlayerWeapon::MELEE:
				AudioManager::Get().PlayAudio(s_WeaponSwitchClaws, false);
				break;

			case PlayerWeapon::GUN:
				AudioManager::Get().PlayAudio(s_WeaponSwitchGuns, false);
				break;
			}

			s_WeaponSwitchSoundPlayed = true;
		}
	}

	//unpause to prevent being stuck on frame
	if (player.spriteSheet->IsPaused()) {
		player.spriteSheet->SetPaused(false);
	}
}

void Player_UpdateMovementAnimation(Player& player, float dt, bool isWalking, bool isInAir)
{
	// --- Decide desired clip ---
	player._desiredClip = "";
	player._forceRestart = false;

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
					player._desiredClip = "claw2normal";
				else if (player.previousWeapon == PlayerWeapon::GUN)
					player._desiredClip = "gun2normal";
			}
			else if (player.weapon == PlayerWeapon::MELEE) {
				// switching to melee
				if (player.previousWeapon == PlayerWeapon::NONE)
					player._desiredClip = "normal2claw";
				else if (player.previousWeapon == PlayerWeapon::GUN)
					player._desiredClip = "gun2claw";
			}
			else if (player.weapon == PlayerWeapon::GUN) {
				// switching to gun
				if (player.previousWeapon == PlayerWeapon::NONE)
					player._desiredClip = "normal2gun";
				else if (player.previousWeapon == PlayerWeapon::MELEE)
					player._desiredClip = "claw2gun";
			}

			// force restart only if we just entered this clip
			if (!player._desiredClip.empty() && player.spriteSheet->GetCurrentClip() != player._desiredClip) {
				player._forceRestart = true;
			}

			// if the switch clip has finished playing, clear the trigger and update previousWeapon
			if (player.weaponSwitchTriggered &&
				player.spriteSheet->GetCurrentClip() == player._desiredClip &&
				!player.spriteSheet->IsPlaying()) {
				player.weaponSwitchTriggered = false;
				player.weaponSwitchInProgress = false;
				player.previousWeapon = player.weapon;
				s_WeaponSwitchSoundPlayed = false;
			}
		}
	}

	// dash animation
	if (player.isDashing && player._desiredClip.empty()) {
		player._desiredClip = "dash";
		player._forceRestart = !player.isDashing;
	}
	// hurt animation 
	else if (player.isHurt && player._desiredClip.empty()) {
		player._desiredClip = "hurt";
		if (!player.wasHurt) player._forceRestart = true;
	}
	// gun attack override everything except hurt
	else if (player.weapon == PlayerWeapon::GUN && player.fireTimer > 0.0f && player._desiredClip.empty())
	{
		player._desiredClip = "gunShoot";

		// restart when firing starts
		if (fabs(player.fireTimer - player.fireCooldown) < 0.001f)
			player._forceRestart = true;

		player.playingTailAnim = false;
		player.idleLoopCount = 0;
	}
	// melee aerial attacks (up/down) animate while jumping
	else if (isInAir && player.weapon == PlayerWeapon::MELEE && player.isAttacking && player._desiredClip.empty()) {
		if (player.slashDirection == SlashDirection::UP)
			player._desiredClip = "clawAttackUP";
		else if (player.slashDirection == SlashDirection::DOWN)
			player._desiredClip = "clawAttackDOWN";
		else
			player._desiredClip = "clawAttack"; // horizontal aerial attack

		if (!player.wasAttacking) player._forceRestart = true;
	}
	// jump animation when in air
	else if (isInAir && !player.weaponSwitchTriggered) {
		// check if moving sideways (A or D held)
		bool movingSideways = AEInputCheckCurr('A') || AEInputCheckCurr('D');
		if (movingSideways) {
			player._desiredClip = "jumpSIDE";
			// if already playing jumpSIDE and reached the end, freeze it
			if (player.spriteSheet->GetCurrentClip() == "jumpSIDE") {
				u32 currentFrame = player.spriteSheet->GetCurrentFrame();
				// frame 17 is the last frame of jumpSIDE (start: 15, end: 17)
				if (currentFrame >= 17) {
					player.spriteSheet->SetPaused(true);
				}
				else {
					player._forceRestart = false;
				}
			}
			else {
				player._forceRestart = true;
			}
		}
		else {
			player._desiredClip = "jumpFRONT";
			// if already playing jumpFRONT and reached the end, freeze it
			if (player.spriteSheet->GetCurrentClip() == "jumpFRONT") {
				u32 currentFrame = player.spriteSheet->GetCurrentFrame();
				// frame 12 is the last frame of jumpFRONT (start: 10, end: 12)
				if (currentFrame >= 12) {
					player._forceRestart = false;
					player.spriteSheet->SetPaused(true);
				}
				else {
					player._forceRestart = false;
				}
			}
			else {
				player._forceRestart = true;
			}
		}
	}
	if (player._desiredClip.empty()) {
		// when landing, unpause the spritesheet
		if (player.spriteSheet->IsPaused()) {
			player.spriteSheet->SetPaused(false);
		}
		// normal state based on weapon and actions
		switch (player.weapon) {
		case PlayerWeapon::NONE:
			if (isWalking) {
				player._desiredClip = "walk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				// idle tail animation logic
				if (player.playingTailAnim) {
					player._desiredClip = "idle2";
					// check if tail animation finished (non-looping clip stops)
					if (player.spriteSheet->GetCurrentClip() == "idle2" &&
						!player.spriteSheet->IsPlaying()) {
						player.playingTailAnim = false;
						player.idleLoopCount = 0;
						player.idleLoopsBeforeTail = 5 + (rand() % 6); // new random count
						player._desiredClip = "idle";
						player._forceRestart = true;
					}
				}
				else {
					player._desiredClip = "idle";
					// track frame changes to detect loop completion
					u32 lastFrame = player.lastFrame;
					u32 currentFrame = player.spriteSheet->GetCurrentFrame();
					// if frame wrapped back to start (loop completed)
					if (currentFrame < lastFrame && player.spriteSheet->GetCurrentClip() == "idle") {
						player.idleLoopCount++;
						// trigger tail animation after random loops
						if (player.idleLoopCount >= player.idleLoopsBeforeTail) {
							player.playingTailAnim = true;
							player._desiredClip = "idle2";
							player._forceRestart = true;
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
					player._desiredClip = "clawAttackUP";
				else
					player._desiredClip = "clawAttack"; // horizontal
				if (!player.wasAttacking) player._forceRestart = true;
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else if (isWalking) {
				player._desiredClip = "clawWalk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				player._desiredClip = "clawIdle";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			break;

		case PlayerWeapon::GUN:
			if (player.fireTimer > 0.0f) {
				player._desiredClip = "gunShoot";

				if (fabs(player.fireTimer - player.fireCooldown) < 0.001f)
					player._forceRestart = true;

				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else if (isWalking) {
				player._desiredClip = "gunWalk";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}
			else {
				player._desiredClip = "gunIdle";
				player.playingTailAnim = false;
				player.idleLoopCount = 0;
			}

			break;
		}
	}

	// Play the clip and update animation
	if (player.spriteSheet && !player._desiredClip.empty()) {
		player.spriteSheet->Play(player._desiredClip, player._forceRestart);
		player.spriteSheet->Update(dt);
	}
}

void Player_HandleLandingDust(Player& player)
{
	// Landing dust — fires once when player touches ground (only when falling, not during upward jumps)
	if (!s_prevGrounded && player.grounded && player.vel.y <= 0.0f) {
		ParticleManager_EmitDust(player.pos.x, player.pos.y - player.height * 0.5f,
			12, 100.f, 220, 220, 220);
	}
	s_prevGrounded = player.grounded;
}

void Player_UpdateStateTrackers(Player& player, bool isWalking)
{
	// Update state trackers
	player.wasAttacking = player.isAttacking;
	player.wasWalking = isWalking;
	player.wasHurt = player.isHurt;

	s_WasGrounded = player.grounded;
}

} // anonymous namespace

void Player_Draw(const Player& player)
{
	// Draw shield sprite
	if (player.shieldActive && player.shieldSprite) {

		float shieldWidth = player.width + SHIELD_WIDTH_OFFSET;
		float shieldHeight = player.height + SHIELD_HEIGHT_OFFSET;

		MeshManager::Get().DrawSpriteSheet(
			*player.shieldSprite,
			player.pos.x,
			player.pos.y,
			shieldWidth,
			shieldHeight,
			0.9f
		);
	}

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
		if (!s_PlayerDamageSoundPlayed)
		{
			AudioManager::Get().PlayAudio(s_PlayerDamageSound, false);
			s_PlayerDamageSoundPlayed = true;
		}

		// trigger hurt animation
		player.isHurt = true;
		player.hurtTimer = player.spriteSheet->GetClipTotalDuration("hurt");
		if (player.hurtTimer < 0.6f) player.hurtTimer = 0.6f;

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
			g_playerDiedBefore = true;
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
			b.active = false;
			// during block, bullet is absorbed but no damage — don't re-trigger hitStun
			if (enemy.type == EnemyType::Boss
				&& enemy.spriteSheet
				&& enemy.spriteSheet->GetCurrentClip() == "block")
			{
				// push player back, no damage, no hitStun
				float blockDir = (player.pos.x < enemy.pos.x) ? -1.0f : 1.0f;
				player.vel.x = blockDir * BLOCK_PUSHBACK_HORIZONTAL;
				player.vel.y = BLOCK_PUSHBACK_VERTICAL;
				player.knockbackTimer = 0.8f;
				player.isHurt = true;
				player.hurtTimer = 0.8f;
				continue;
			}
			float knockbackDir = (b.vel.x > 0.0f) ? 1.0f : -1.0f;
			Enemy_OnHit(enemy, b.damage, knockbackDir, &player);
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
	// free shield particles
	for (int i = 0; i < 4; ++i) {
		if (player.shieldEmitters[i] != INVALID_EMITTER) {
			ParticleManager_EmitterStop(player.shieldEmitters[i]);
			player.shieldEmitters[i] = INVALID_EMITTER;
		}
	}

	// free dash trail particles
	if (player.dashEmitter != INVALID_EMITTER) {
		ParticleManager_EmitterStop(player.dashEmitter);
		player.dashEmitter = INVALID_EMITTER;
	}

	// free bullets

	//free bullet internal resources (if any)
	for (auto& b : player.bullets)
		PlayerBullet_Free(b);

	//release vector heap memory
	player.bullets.clear();
	player.bullets.shrink_to_fit();

}

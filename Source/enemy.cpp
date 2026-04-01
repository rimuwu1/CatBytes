/* Start Header ************************************************************************/
/*!
\file enemy.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
        Sim Hui Min, s.huimin, 2503506
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
        kerwinjiajie.wong@digipen.edu
        s.huimin@digipen.edu
\date Junuary, 24, 2026
\brief Implements a simple patrolling(?) enemy.
The enemy moves automatically left and right between patrol bounds

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "enemy.h"
#include "ObjectManager.h"
#include "MeshManager.h"
#include "PhysicsManager.h"
#include "Player.h"
#include "Buff.h"
#include "AudioManager.h"
#include "Camera.h"
#include "SpriteSheet.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"


static AEAudio s_EasyEnemyAttackSound{};
static AEAudio s_HardEnemyAttackSound{};
static AEAudio s_EnemyDeath{};
static AEAudio s_HardEnemyDeath{};
static bool s_WalkPlaying = false;
static bool s_HardWalkPlaying = false;
static AEAudio s_EasyDamage{};
static AEAudio s_HardDamage{};
static AEAudio s_BossDamage{};
static AEAudio s_BossAttackSound{};

static bool Enemy_IsInCamera(const Enemy& enemy)
{
    const float halfScreenW = 1600.0f * 0.5f;
    const float halfScreenH = 900.0f * 0.5f;

    const float camLeft = globalCam.x - halfScreenW;
    const float camRight = globalCam.x + halfScreenW;
    const float camBottom = globalCam.y - halfScreenH;
    const float camTop = globalCam.y + halfScreenH;

    const float enemyLeft = enemy.pos.x - enemy.width * 0.5f;
    const float enemyRight = enemy.pos.x + enemy.width * 0.5f;
    const float enemyBottom = enemy.pos.y - enemy.height * 0.5f;
    const float enemyTop = enemy.pos.y + enemy.height * 0.5f;

    const bool overlapX = (enemyRight >= camLeft) && (enemyLeft <= camRight);
    const bool overlapY = (enemyTop >= camBottom) && (enemyBottom <= camTop);

    return overlapX && overlapY;
}

static void Enemy_SetState(Enemy& enemy, EnemyState newState)
{
    enemy.state = newState;

    switch (newState)
    {
    case EnemyState::Idle:
        enemy.stateTimer = enemy.idleDuration;
        enemy.vel.x = 0.0f;

        if (enemy.spriteSheet && enemy.spriteSheet->GetCurrentClip() != "idle")
            enemy.spriteSheet->Play("idle");
        break;

    case EnemyState::Patrol:
        enemy.stateTimer = 0.0f;

        if (enemy.spriteSheet && enemy.spriteSheet->GetCurrentClip() != "patrol")
            enemy.spriteSheet->Play("patrol");
        break;

    case EnemyState::Attack:
        enemy.vel.x = 0.0f;

        enemy.hasAppliedAttackDamage = false;

        if (enemy.spriteSheet && enemy.spriteSheet->GetCurrentClip() != "attack")
            enemy.spriteSheet->Play("attack", true);

        if (enemy.spriteSheet)
            enemy.stateTimer = enemy.spriteSheet->GetClipTotalDuration("attack");
        else
            enemy.stateTimer = 0.3f;
        break;
    }
}

static float Enemy_GetSpawnYFromPlatform(float platformCenterY, float platformHeight, float enemyHeight)
{
    constexpr float ENEMY_VISUAL_Y_OFFSET = 1.0f;
    return platformCenterY + (platformHeight * 0.5f) + (enemyHeight * 0.5f) + ENEMY_VISUAL_Y_OFFSET;
}

// -----------------------------------------------------------------------------
// initialize easy enemy
// sets position, size, direction, alive status, loads speed and textures
// -----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, const rapidjson::Value& config) {
    enemy.facesLeft = true;

    s_EasyEnemyAttackSound = AudioManager::Get().GetAudio("easy_enemy_attack");
    s_EnemyDeath = AudioManager::Get().GetAudio("enemy_death_easy");
    s_EasyDamage = AudioManager::Get().GetAudio("damage_easy");

    if (config.HasMember("x") && config["x"].IsFloat())
        enemy.pos.x = config["x"].GetFloat();
    else {
        enemy.pos.x = 0.0f;
        printf("Warning: Enemy missing 'x', defaulting to 0\n");
    }

    // Size
    if (config.HasMember("width") && config["width"].IsFloat())
        enemy.width = config["width"].GetFloat();
    else {
        enemy.width = 80.0f;
        printf("Warning: Enemy missing 'width', defaulting to 80\n");
    }
    if (config.HasMember("height") && config["height"].IsFloat())
        enemy.height = config["height"].GetFloat();
    else {
        enemy.height = 80.0f;
        printf("Warning: Enemy missing 'height', defaulting to 80\n");
    }

    // Position (required, but provide fallback)
    if (config.HasMember("platform_y") && config["platform_y"].IsFloat())
    {
        enemy.platformY = config["platform_y"].GetFloat();
        enemy.pos.y = Enemy_GetSpawnYFromPlatform(enemy.platformY, 40.0f, enemy.height);
    }
    else if (config.HasMember("y") && config["y"].IsFloat())
    {
        enemy.pos.y = config["y"].GetFloat();
        enemy.platformY = enemy.pos.y - (40.0f * 0.5f) - (enemy.height * 0.5f);
    }
    else
    {
        enemy.platformY = 0.0f;
        enemy.pos.y = Enemy_GetSpawnYFromPlatform(enemy.platformY, 40.0f, enemy.height);
        printf("Warning: Enemy missing 'platform_y'/'y', defaulting to 0\n");
    }


    // Movement speed
    if (config.HasMember("speed") && config["speed"].IsFloat())
        enemy.moveSpeed = config["speed"].GetFloat();
    else {
        enemy.moveSpeed = 100.0f;
        printf("Warning: Enemy missing 'speed', defaulting to 100\n");
    }

    // Hit points
    if (config.HasMember("hp") && config["hp"].IsFloat()) {
        enemy.hitPoints = config["hp"].GetFloat();
        enemy.maxHitPoints = enemy.hitPoints;
    }
    else {
        enemy.hitPoints = 3.0f;
        printf("Warning: Enemy missing 'hp', defaulting to 3\n");
    }

    // Shooting parameters (optional)
    if (config.HasMember("shoot_cooldown") && config["shoot_cooldown"].IsFloat())
        enemy.shootCooldown = config["shoot_cooldown"].GetFloat();
    else
        enemy.shootCooldown = 1.0f;

    if (config.HasMember("bullet_speed") && config["bullet_speed"].IsFloat())
        enemy.bulletSpeed = config["bullet_speed"].GetFloat();
    else
        enemy.bulletSpeed = 400.0f; // default

    if (config.HasMember("bullet_damage") && config["bullet_damage"].IsFloat())
        enemy.bulletDamage = config["bullet_damage"].GetFloat();
    else
        enemy.bulletDamage = 1.0f;

    if (config.HasMember("bullet_range") && config["bullet_range"].IsFloat())
        enemy.bulletRange = config["bullet_range"].GetFloat();
    else
        enemy.bulletRange = 1600.0f;

    if (config.HasMember("bullet_width") && config["bullet_width"].IsFloat())
        enemy.bulletWidth = config["bullet_width"].GetFloat();
    else
        enemy.bulletWidth = 30.0f;

    if (config.HasMember("bullet_height") && config["bullet_height"].IsFloat())
        enemy.bulletHeight = config["bullet_height"].GetFloat();
    else
        enemy.bulletHeight = 30.0f;

    enemy.shootTimer = enemy.shootCooldown;
    enemy.vel = { 0.0f, 0.0f };

    if (config.HasMember("start_direction") && config["start_direction"].IsInt())
        enemy.direction = (config["start_direction"].GetInt() < 0) ? -1 : 1;
    else
        enemy.direction = 1;

    enemy.homeDirection = enemy.direction;

    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.justDied = true;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Easy;
    enemy.homeX = enemy.pos.x;

    if (config.HasMember("return_to_home_only") && config["return_to_home_only"].IsBool())
        enemy.returnToHomeOnly = config["return_to_home_only"].GetBool();
    else
        enemy.returnToHomeOnly = false;

    // Patrol bounds
    if (config.HasMember("patrol_min_x") && config["patrol_min_x"].IsFloat())
        enemy.patrolMinX = config["patrol_min_x"].GetFloat();
    else
        enemy.patrolMinX = enemy.pos.x - 100.0f;

    if (config.HasMember("patrol_max_x") && config["patrol_max_x"].IsFloat())
        enemy.patrolMaxX = config["patrol_max_x"].GetFloat();
    else
        enemy.patrolMaxX = enemy.pos.x + 100.0f;

    //hit cooldown
    if (config.HasMember("hit_cooldown") && config["hit_cooldown"].IsFloat())
        enemy.hitCooldown = config["hit_cooldown"].GetFloat();
    else
        enemy.hitCooldown = 0.2f;

    enemy.hitCooldownTimer = 0.0f;

    //SpriteSheet Loading
    if (config.HasMember("animations"))
    {
        const auto& anims = config["animations"];

        enemy.spriteSheet = std::make_unique<SpriteSheet>(
            anims["file"].GetString(),
            anims["rows"].GetInt(),
            anims["cols"].GetInt()
        );

        const auto& clips = anims["clips"];
        for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
        {
            const auto& c = clips[i];
            enemy.spriteSheet->AddClip(
                c["name"].GetString(),
                c["start"].GetInt(),
                c["end"].GetInt(),
                c["duration"].GetFloat(),
                c["loop"].GetBool()
            );
        }

        enemy.spriteSheet->Play("patrol");
    }

    // Enemy bullet sprite
    if (config.HasMember("bullet_animations")) {
        const auto& anims = config["bullet_animations"];
        enemy.bulletSprite = std::make_unique<SpriteSheet>(
            anims["file"].GetString(),
            anims["rows"].GetInt(),
            anims["cols"].GetInt()
        );
        const auto& clips = anims["clips"];
        for (rapidjson::SizeType i = 0; i < clips.Size(); i++) {
            const auto& c = clips[i];
            enemy.bulletSprite->AddClip(
                c["name"].GetString(),
                c["start"].GetInt(),
                c["end"].GetInt(),
                c["duration"].GetFloat(),
                c["loop"].GetBool()
            );
        }
        enemy.bulletSprite->Play("fly");
    }

    // Knockback
    if (config.HasMember("knockback_velocity") && config["knockback_velocity"].IsFloat())
        enemy.knockbackVelocity = config["knockback_velocity"].GetFloat();
    else
        enemy.knockbackVelocity = 300.0f;
    enemy.knockbackVel = { 0.0f, 0.0f };
    enemy.knockbackTimer = 0.0f;

    if (config.HasMember("idle_duration") && config["idle_duration"].IsFloat())
        enemy.idleDuration = config["idle_duration"].GetFloat();
    else
        enemy.idleDuration = 1.0f;

    Enemy_SetState(enemy, EnemyState::Patrol);
}

void HardEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {

    s_HardEnemyAttackSound = AudioManager::Get().GetAudio("hard_enemy_attack");
    s_HardEnemyDeath = AudioManager::Get().GetAudio("enemy_death_hard");
    s_HardDamage = AudioManager::Get().GetAudio("damage_hard");

    if (config.HasMember("x") && config["x"].IsFloat())
        enemy.pos.x = config["x"].GetFloat();
    else {
        enemy.pos.x = 0.0f;
        printf("Warning: Enemy missing 'x', defaulting to 0\n");
    }

    // Size
    if (config.HasMember("width") && config["width"].IsFloat())
        enemy.width = config["width"].GetFloat();
    else {
        enemy.width = 80.0f;
        printf("Warning: HardEnemy missing 'width', defaulting to 80\n");
    }
    if (config.HasMember("height") && config["height"].IsFloat())
        enemy.height = config["height"].GetFloat();
    else {
        enemy.height = 80.0f;
        printf("Warning: HardEnemy missing 'height', defaulting to 80\n");
    }

    // Position
    if (config.HasMember("platform_y") && config["platform_y"].IsFloat())
    {
        enemy.platformY = config["platform_y"].GetFloat();
        enemy.pos.y = Enemy_GetSpawnYFromPlatform(enemy.platformY, 40.0f, enemy.height);
    }
    else if (config.HasMember("y") && config["y"].IsFloat())
    {
        enemy.pos.y = config["y"].GetFloat();
        enemy.platformY = enemy.pos.y - (40.0f * 0.5f) - (enemy.height * 0.5f);
    }
    else
    {
        enemy.platformY = 0.0f;
        enemy.pos.y = Enemy_GetSpawnYFromPlatform(enemy.platformY, 40.0f, enemy.height);
        printf("Warning: HardEnemy missing 'platform_y'/'y', defaulting to 0\n");
    }


    // Movement speed
    if (config.HasMember("speed") && config["speed"].IsFloat())
        enemy.moveSpeed = config["speed"].GetFloat();
    else {
        enemy.moveSpeed = 150.0f;
        printf("Warning: HardEnemy missing 'speed', defaulting to 150\n");
    }

    // Hit points
    if (config.HasMember("hp") && config["hp"].IsFloat()) {
        enemy.hitPoints = config["hp"].GetFloat();
        enemy.maxHitPoints = enemy.hitPoints;
    }
    else {
        enemy.hitPoints = 5.0f;
        printf("Warning: HardEnemy missing 'hp', defaulting to 5\n");
    }

    // Collision damage
    if (config.HasMember("damage") && config["damage"].IsFloat())
        enemy.damage = config["damage"].GetFloat();
    else {
        enemy.damage = 3.0f;
        printf("Warning: HardEnemy missing 'damage', defaulting to 3\n");
    }

    // hard enemy melee cooldown/hit timing
    if (config.HasMember("melee_cooldown") && config["melee_cooldown"].IsFloat())
        enemy.meleeCooldown = config["melee_cooldown"].GetFloat();
    else
        enemy.meleeCooldown = 1.0f;

    enemy.meleeCooldownTimer = 0.0f;
    enemy.hasAppliedAttackDamage = false;

    if (config.HasMember("attack_hit_time") && config["attack_hit_time"].IsFloat())
        enemy.attackHitTime = config["attack_hit_time"].GetFloat();
    else
        enemy.attackHitTime = 0.2f;

    if (config.HasMember("attack_range_x") && config["attack_range_x"].IsFloat())
        enemy.attackRangeX = config["attack_range_x"].GetFloat();
    else
        enemy.attackRangeX = enemy.width * 0.75f + 40.0f;

    if (config.HasMember("attack_range_y") && config["attack_range_y"].IsFloat())
        enemy.attackRangeY = config["attack_range_y"].GetFloat();
    else
        enemy.attackRangeY = enemy.height;

    enemy.shootCooldown = 0.0f; // no shooting
    enemy.vel = { 0.0f, 0.0f };

    if (config.HasMember("start_direction") && config["start_direction"].IsInt())
        enemy.direction = (config["start_direction"].GetInt() < 0) ? -1 : 1;
    else
        enemy.direction = 1;

    enemy.homeDirection = enemy.direction;

    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.justDied = true;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Hard;

    enemy.homeX = enemy.pos.x;

    if (config.HasMember("return_to_home_only") && config["return_to_home_only"].IsBool())
        enemy.returnToHomeOnly = config["return_to_home_only"].GetBool();
    else
        enemy.returnToHomeOnly = false;

    // Patrol bounds
    if (config.HasMember("patrol_min_x") && config["patrol_min_x"].IsFloat())
        enemy.patrolMinX = config["patrol_min_x"].GetFloat();
    else
        enemy.patrolMinX = enemy.pos.x - 100.0f;

    if (config.HasMember("patrol_max_x") && config["patrol_max_x"].IsFloat())
        enemy.patrolMaxX = config["patrol_max_x"].GetFloat();
    else
        enemy.patrolMaxX = enemy.pos.x + 100.0f;

    // Knockback
    if (config.HasMember("knockback_velocity") && config["knockback_velocity"].IsFloat())
        enemy.knockbackVelocity = config["knockback_velocity"].GetFloat();
    else
        enemy.knockbackVelocity = 300.0f;
    enemy.knockbackVel = { 0.0f, 0.0f };
    enemy.knockbackTimer = 0.0f;

    //hit cooldown
    if (config.HasMember("hit_cooldown") && config["hit_cooldown"].IsFloat())
        enemy.hitCooldown = config["hit_cooldown"].GetFloat();
    else
        enemy.hitCooldown = 0.2f;

    enemy.hitCooldownTimer = 0.0f;

    //SpriteSheet Loading
    if (config.HasMember("animations"))
    {
        const auto& anims = config["animations"];

        enemy.spriteSheet = std::make_unique<SpriteSheet>(
            anims["file"].GetString(),
            anims["rows"].GetInt(),
            anims["cols"].GetInt()
        );

        const auto& clips = anims["clips"];
        for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
        {
            const auto& c = clips[i];
            enemy.spriteSheet->AddClip(
                c["name"].GetString(),
                c["start"].GetInt(),
                c["end"].GetInt(),
                c["duration"].GetFloat(),
                c["loop"].GetBool()
            );
        }

        enemy.spriteSheet->Play("patrol");
    }

    if (config.HasMember("idle_duration") && config["idle_duration"].IsFloat())
        enemy.idleDuration = config["idle_duration"].GetFloat();
    else
        enemy.idleDuration = 1.0f;

    enemy.state = EnemyState::Patrol;
    enemy.stateTimer = 0.0f;
}

void BossEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {

    enemy.facesLeft = false;

    s_BossDamage = AudioManager::Get().GetAudio("damage_boss");
    s_BossAttackSound = AudioManager::Get().GetAudio("boss_attack");
    // Position
    if (config.HasMember("x") && config["x"].IsFloat())
        enemy.pos.x = config["x"].GetFloat();
    else {
        enemy.pos.x = 0.0f;
        printf("Warning: Boss missing 'x', defaulting to 0\n");
    }
    if (config.HasMember("y") && config["y"].IsFloat())
        enemy.pos.y = config["y"].GetFloat();
    else {
        enemy.pos.y = 0.0f;
        printf("Warning: Boss missing 'y', defaulting to 0\n");
    }

    // Size
    if (config.HasMember("width") && config["width"].IsFloat())
        enemy.width = config["width"].GetFloat();
    else {
        enemy.width = 120.0f;
        printf("Warning: Boss missing 'width', defaulting to 120\n");
    }
    if (config.HasMember("height") && config["height"].IsFloat())
        enemy.height = config["height"].GetFloat();
    else {
        enemy.height = 120.0f;
        printf("Warning: Boss missing 'height', defaulting to 120\n");
    }

    // Movement speed
    if (config.HasMember("speed") && config["speed"].IsFloat())
        enemy.moveSpeed = config["speed"].GetFloat();
    else {
        enemy.moveSpeed = 120.0f;
        printf("Warning: Boss missing 'speed', defaulting to 120\n");
    }

    // Hit points
    if (config.HasMember("hp") && config["hp"].IsFloat()) {
        enemy.hitPoints = config["hp"].GetFloat();
        enemy.maxHitPoints = enemy.hitPoints;
    }
    else {
        enemy.hitPoints = 30.0f;
        enemy.maxHitPoints = enemy.hitPoints;
        printf("Warning: Boss missing 'hp', defaulting to 30\n");
    }

    // Collision damage
    if (config.HasMember("damage") && config["damage"].IsFloat())
        enemy.damage = config["damage"].GetFloat();
    else if (config.HasMember("damage") && config["damage"].IsInt())
        enemy.damage = static_cast<float>(config["damage"].GetInt());
    else {
        enemy.damage = 8.0f;
        printf("Warning: Boss missing 'damage', defaulting to 8\n");
    }

    enemy.shootCooldown = 0.0f;
    enemy.vel = { 0.0f, 0.0f };

    if (config.HasMember("start_direction") && config["start_direction"].IsInt())
        enemy.direction = (config["start_direction"].GetInt() < 0) ? -1 : 1;
    else
        enemy.direction = 1;

    enemy.homeDirection = enemy.direction;

    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.justDied = true;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Boss;

    enemy.homeX = enemy.pos.x;

    if (config.HasMember("return_to_home_only") && config["return_to_home_only"].IsBool())
        enemy.returnToHomeOnly = config["return_to_home_only"].GetBool();
    else
        enemy.returnToHomeOnly = false;

    // Patrol bounds
    if (config.HasMember("patrol_min_x") && config["patrol_min_x"].IsFloat())
        enemy.patrolMinX = config["patrol_min_x"].GetFloat();
    else
        enemy.patrolMinX = enemy.pos.x - 200.0f;

    if (config.HasMember("patrol_max_x") && config["patrol_max_x"].IsFloat())
        enemy.patrolMaxX = config["patrol_max_x"].GetFloat();
    else
        enemy.patrolMaxX = enemy.pos.x + 200.0f;

    // Knockback
    if (config.HasMember("knockback_velocity") && config["knockback_velocity"].IsFloat())
        enemy.knockbackVelocity = config["knockback_velocity"].GetFloat();
    else
        enemy.knockbackVelocity = 300.0f;
    enemy.knockbackVel = { 0.0f, 0.0f };
    enemy.knockbackTimer = 0.0f;

    // SpriteSheet Loading
    if (config.HasMember("animations"))
    {
        const auto& anims = config["animations"];

        enemy.spriteSheet = std::make_unique<SpriteSheet>(
            anims["file"].GetString(),
            anims["rows"].GetInt(),
            anims["cols"].GetInt()
        );

        const auto& clips = anims["clips"];
        for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
        {
            const auto& c = clips[i];
            enemy.spriteSheet->AddClip(
                c["name"].GetString(),
                c["start"].GetInt(),
                c["end"].GetInt(),
                c["duration"].GetFloat(),
                c["loop"].GetBool()
            );
        }

        enemy.spriteSheet->Play("walk");
    }

    if (config.HasMember("idle_duration") && config["idle_duration"].IsFloat())
        enemy.idleDuration = config["idle_duration"].GetFloat();
    else
        enemy.idleDuration = 1.0f;

    enemy.state = EnemyState::Patrol;
    enemy.stateTimer = 0.0f;

    // Load shared laser timing from config if present
    if (config.HasMember("laser_cooldown"))      enemy.laserCooldown = config["laser_cooldown"].GetFloat();
    if (config.HasMember("laser_track"))         enemy.laserTrackDuration = config["laser_track"].GetFloat();
    if (config.HasMember("laser_lockon"))        enemy.laserLockDuration = config["laser_lockon"].GetFloat();
    if (config.HasMember("laser_fire"))          enemy.laserFireDuration = config["laser_fire"].GetFloat();
    if (config.HasMember("laser_damage"))        enemy.laserDamage = config["laser_damage"].GetFloat();
    if (config.HasMember("laser_knockback"))     enemy.laserKnockback = config["laser_knockback"].GetFloat();
    // Load laser texture
    enemy.laserTex = TextureManager::Get().LoadTexture("Assets/Images/laserObstacle.png");

    // Load boss laser emitter origins from config
    if (config.HasMember("boss_lasers") && config["boss_lasers"].IsArray()) {
        for (const auto& l : config["boss_lasers"].GetArray()) {
            BossLaser laser;
            laser.origin.x = l.HasMember("x") ? l["x"].GetFloat() : 0.0f;
            laser.origin.y = l.HasMember("y") ? l["y"].GetFloat() : 0.0f;
            laser.width = l.HasMember("width") ? l["width"].GetFloat() : 16.0f;
            laser.state = BossLaserState::Inactive;
            laser.cooldownTimer = enemy.laserCooldown;

            // Stagger lasers so they don't all fire at once:
            // offset each laser's initial cooldown by its index * (laserCooldown / count)
            enemy.bossLasers.push_back(laser);
        }
        // Apply stagger offsets after all lasers are added
        const float count = static_cast<float>(enemy.bossLasers.size());
        for (size_t i = 0; i < enemy.bossLasers.size(); ++i) {
            enemy.bossLasers[i].cooldownTimer = enemy.laserCooldown * (static_cast<float>(i) / count);
        }
    }
}

// -----------------------------------------------------------------------------
// Update enemy: automatic left/right patrol
// -----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt) {

    enemy.justDied = false;

    if (enemy.hitCooldownTimer > 0.0f)
    {
        enemy.hitCooldownTimer -= dt;
        if (enemy.hitCooldownTimer < 0.0f)
            enemy.hitCooldownTimer = 0.0f;
    }

    PhysicsManager& physics = PhysicsManager::Get();

    // Apply gravity
    physics.ApplyGravity(enemy.vel.y, enemy.isGrounded, dt);

    // Clamp fall speed
    physics.ClampFallSpeed(enemy.vel.y);

    if (!enemy.spriteSheet) return;

    const std::string currentClip = enemy.spriteSheet->GetCurrentClip();

    // keep dead animation running until it finishes, then stop drawing
    if (!enemy.isAlive)
    {
        if (currentClip == "dead" && enemy.hitStunTimer > 0.0f)
        {
            if (!enemy.justDied) return;
            if (enemy.justDied)
            {
                if (enemy.type == EnemyType::Easy)
                    AudioManager::Get().PlayAudio(s_EnemyDeath, false);
                else if (enemy.type == EnemyType::Hard)
                    AudioManager::Get().PlayAudio(s_HardEnemyDeath, false);

                enemy.justDied = false;
            }
        }
        if (currentClip == "dead")
        {
            enemy.hitStunTimer -= dt;
            enemy.spriteSheet->Update(dt);

            if (enemy.hitStunTimer <= 0.0f)
            {
                enemy.hitStunTimer = 0.0f;
                enemy.spriteSheet->Stop();
            }
        }
        return;
    }

    // Attack/hit state
    if (enemy.hitStunTimer > 0.0f)
    {
        if (enemy.knockbackTimer > 0.0f)
        {
            enemy.knockbackTimer -= dt;
            enemy.vel.x = enemy.knockbackVel.x;
            if (enemy.knockbackTimer <= 0.0f)
            {
                enemy.knockbackTimer = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
            }
        }
        else
        {
            enemy.hitStunTimer -= dt;
            enemy.vel.x = 0.0f;
        }

        // Integrate Y normally
        enemy.pos.y += enemy.vel.y * dt;

        // Integrate X, but clamp to patrol/platform bounds
        const float halfWidth = enemy.width * 0.5f;
        const float minCenterX = enemy.patrolMinX + halfWidth;
        const float maxCenterX = enemy.patrolMaxX - halfWidth;

        float nextX = enemy.pos.x + enemy.vel.x * dt;

        if (nextX <= minCenterX)
        {
            enemy.pos.x = minCenterX;
            enemy.direction = 1;
            enemy.vel.x = 0.0f;
            enemy.knockbackVel = { 0.0f, 0.0f };
            enemy.knockbackTimer = 0.0f;
        }
        else if (nextX >= maxCenterX)
        {
            enemy.pos.x = maxCenterX;
            enemy.direction = -1;
            enemy.vel.x = 0.0f;
            enemy.knockbackVel = { 0.0f, 0.0f };
            enemy.knockbackTimer = 0.0f;
        }
        else
        {
            enemy.pos.x = nextX;
        }

        // Ground collision
        float groundY = enemy.platformY + (40.0f * 0.5f) + (enemy.height * 0.5f);

        if (enemy.pos.y <= groundY)
        {
            enemy.pos.y = groundY;
            enemy.vel.y = 0.0f;
            enemy.isGrounded = true;
        }
        else
        {
            enemy.isGrounded = false;
        }

        enemy.spriteSheet->Update(dt);

        if (enemy.hitStunTimer <= 0.0f)
        {
            enemy.hitStunTimer = 0.0f;
            Enemy_SetState(enemy, EnemyState::Patrol);
        }
        return;
    }

    if (enemy.shootCooldown > 0.0f)
    {
        bool canShoot = false;

        if (enemy.returnToHomeOnly)
        {
            const float epsilon = 1.0f;
            const bool atHome = fabsf(enemy.homeX - enemy.pos.x) <= epsilon;
            canShoot = atHome && (enemy.state == EnemyState::Idle);
        }
        else
        {
            canShoot = (enemy.state == EnemyState::Patrol);
        }

        if (canShoot)
        {
            enemy.shootTimer -= dt;
            if (enemy.shootTimer <= 0.0f)
            {
                Enemy_SetState(enemy, EnemyState::Attack);
                if (enemy.type == EnemyType::Easy)
                {
                    ObjectManager::Get().SpawnEnemyBullet(
                        enemy,
                        enemy.bulletSpeed,
                        enemy.bulletDamage,
                        enemy.bulletRange
                    );

                    if (Enemy_IsInCamera(enemy))
                        AudioManager::Get().PlayAudio(s_EasyEnemyAttackSound, false);

                    enemy.shootTimer = enemy.shootCooldown;
                }
            }
        }
    }

    switch (enemy.state)
    {
    case EnemyState::Idle:
        enemy.stateTimer -= dt;
        enemy.vel.x = 0.0f;

        if (enemy.stateTimer <= 0.0f)
            Enemy_SetState(enemy, EnemyState::Patrol);
        break;

    case EnemyState::Patrol:
    {
        if (enemy.returnToHomeOnly)
        {
            const float dxToHome = enemy.homeX - enemy.pos.x;
            const float epsilon = 1.0f;

            if (fabsf(dxToHome) <= epsilon)
            {
                enemy.pos.x = enemy.homeX;
                enemy.direction = enemy.homeDirection;
                enemy.vel = { 0.0f, enemy.vel.y };
                Enemy_SetState(enemy, EnemyState::Idle);
            }
            else
            {
                enemy.direction = (dxToHome > 0.0f) ? 1 : -1;
                enemy.vel.x = enemy.direction * enemy.moveSpeed;
            }
        }
        else
        {
            enemy.vel.x = enemy.direction * enemy.moveSpeed;

            const float halfWidth = enemy.width * 0.5f;
            const float minCenterX = enemy.patrolMinX + halfWidth;
            const float maxCenterX = enemy.patrolMaxX - halfWidth;

            const float nextX = enemy.pos.x + enemy.vel.x * dt;

            if (nextX <= minCenterX)
            {
                enemy.pos.x = minCenterX;
                enemy.direction = 1;
                enemy.vel.x = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
                enemy.knockbackTimer = 0.0f;
                Enemy_SetState(enemy, EnemyState::Idle);
            }
            else if (nextX >= maxCenterX)
            {
                enemy.pos.x = maxCenterX;
                enemy.direction = -1;
                enemy.vel.x = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
                enemy.knockbackTimer = 0.0f;
                Enemy_SetState(enemy, EnemyState::Idle);
            }
        }
        break;
    }

    case EnemyState::Attack:
        enemy.stateTimer -= dt;
        enemy.vel.x = 0.0f;

        if (enemy.stateTimer <= 0.0f)
        {
            if (enemy.returnToHomeOnly)
                Enemy_SetState(enemy, EnemyState::Idle);
            else
                Enemy_SetState(enemy, EnemyState::Patrol);
        }
        break;
    }

    if (enemy.knockbackTimer <= 0.0f && enemy.state != EnemyState::Patrol)
        enemy.vel.x = 0.0f;

    // Apply movement (X + Y)
    physics.Integrate(enemy.pos, enemy.vel, dt);

    // Ground collision (platform)
    float groundY = enemy.platformY + (40.0f * 0.5f) + (enemy.height * 0.5f);

    if (enemy.pos.y <= groundY)
    {
        enemy.pos.y = groundY;
        enemy.vel.y = 0.0f;
        enemy.isGrounded = true;
    }
    else
    {
        enemy.isGrounded = false;
    }
    enemy.spriteSheet->Update(dt);
}

void HardEnemy_Update(Enemy& enemy, float dt) {
    enemy.justDied = false;

    if (enemy.hitCooldownTimer > 0.0f)
    {
        enemy.hitCooldownTimer -= dt;
        if (enemy.hitCooldownTimer < 0.0f)
            enemy.hitCooldownTimer = 0.0f;
    }

    if (enemy.meleeCooldownTimer > 0.0f)
    {
        enemy.meleeCooldownTimer -= dt;
        if (enemy.meleeCooldownTimer < 0.0f)
            enemy.meleeCooldownTimer = 0.0f;
    }

    PhysicsManager& physics = PhysicsManager::Get();

    physics.ApplyGravity(enemy.vel.y, enemy.isGrounded, dt);
    physics.ClampFallSpeed(enemy.vel.y);

    if (!enemy.spriteSheet)
        return;

    const std::string currentClip = enemy.spriteSheet->GetCurrentClip();

    // Dead, keep dead animation running until it finishes
    if (!enemy.isAlive)
    {
        if (currentClip == "dead")
        {
            enemy.hitStunTimer -= dt;
            enemy.spriteSheet->Update(dt);

            if (enemy.hitStunTimer <= 0.0f)
            {
                enemy.hitStunTimer = 0.0f;
                enemy.spriteSheet->Stop();// stop animation
            }
        }
        return;
    }

    // attack/hit state
    if (enemy.hitStunTimer > 0.0f)
    {
        if (enemy.knockbackTimer > 0.0f)
        {
            enemy.knockbackTimer -= dt;
            enemy.vel.x = enemy.knockbackVel.x;
            if (enemy.knockbackTimer <= 0.0f)
            {
                enemy.knockbackTimer = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
            }
        }
        else
        {
            enemy.hitStunTimer -= dt;
            enemy.vel.x = 0.0f;
        }


        // Integrate Y normally
        enemy.pos.y += enemy.vel.y * dt;

        // Integrate X, but clamp to patrol/platform bounds
        const float halfWidth = enemy.width * 0.5f;
        const float minCenterX = enemy.patrolMinX + halfWidth;
        const float maxCenterX = enemy.patrolMaxX - halfWidth;

        float nextX = enemy.pos.x + enemy.vel.x * dt;

        if (nextX <= minCenterX)
        {
            enemy.pos.x = minCenterX;
            enemy.vel.x = 0.0f;
            enemy.knockbackVel = { 0.0f, 0.0f };
            enemy.knockbackTimer = 0.0f;
        }
        else if (nextX >= maxCenterX)
        {
            enemy.pos.x = maxCenterX;
            enemy.vel.x = 0.0f;
            enemy.knockbackVel = { 0.0f, 0.0f };
            enemy.knockbackTimer = 0.0f;
        }
        else
        {
            enemy.pos.x = nextX;
        }

        // Ground collision
        float groundY = enemy.platformY + (40.0f * 0.5f) + (enemy.height * 0.5f);

        if (enemy.pos.y <= groundY)
        {
            enemy.pos.y = groundY;
            enemy.vel.y = 0.0f;
            enemy.isGrounded = true;
        }
        else
        {
            enemy.isGrounded = false;
        }

        enemy.spriteSheet->Update(dt);

        if (enemy.hitStunTimer <= 0.0f)
        {
            enemy.hitStunTimer = 0.0f;
            Enemy_SetState(enemy, EnemyState::Patrol);
        }
        return;
    }

    switch (enemy.state)
    {
    case EnemyState::Idle:
        enemy.stateTimer -= dt;
        enemy.vel.x = 0.0f;

        if (enemy.stateTimer <= 0.0f)
            Enemy_SetState(enemy, EnemyState::Patrol);
        break;

    case EnemyState::Patrol:
    {
        if (enemy.returnToHomeOnly)
        {
            const float dxToHome = enemy.homeX - enemy.pos.x;
            const float epsilon = 1.0f;

            if (fabsf(dxToHome) <= epsilon)
            {
                enemy.pos.x = enemy.homeX;
                enemy.direction = enemy.homeDirection;
                enemy.vel = { 0.0f, enemy.vel.y };
                Enemy_SetState(enemy, EnemyState::Idle);
            }
            else
            {
                enemy.direction = (dxToHome > 0.0f) ? 1 : -1;
                enemy.vel.x = enemy.direction * enemy.moveSpeed;
            }
        }
        else
        {
            enemy.vel.x = enemy.direction * enemy.moveSpeed;

            const float halfWidth = enemy.width * 0.5f;
            const float minCenterX = enemy.patrolMinX + halfWidth;
            const float maxCenterX = enemy.patrolMaxX - halfWidth;

            const float nextX = enemy.pos.x + enemy.vel.x * dt;

            if (nextX <= minCenterX)
            {
                enemy.pos.x = minCenterX;
                enemy.direction = 1;
                enemy.vel.x = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
                enemy.knockbackTimer = 0.0f;
                Enemy_SetState(enemy, EnemyState::Idle);
            }
            else if (nextX >= maxCenterX)
            {
                enemy.pos.x = maxCenterX;
                enemy.direction = -1;
                enemy.vel.x = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
                enemy.knockbackTimer = 0.0f;
                Enemy_SetState(enemy, EnemyState::Idle);
            }
        }
        break;
    }

    case EnemyState::Attack:
    {
        enemy.stateTimer -= dt;
        enemy.vel.x = 0.0f;

        float attackDuration = enemy.spriteSheet
            ? enemy.spriteSheet->GetClipTotalDuration("attack")
            : 0.3f;

        float elapsed = attackDuration - enemy.stateTimer;

        if (!enemy.hasAppliedAttackDamage && elapsed >= enemy.attackHitTime)
        {
            Player& player = ObjectManager::Get().GetPlayer();

            const float dx = player.pos.x - enemy.pos.x;
            const float dy = fabsf(player.pos.y - enemy.pos.y);

            const bool inFront =
                (enemy.direction > 0 && dx >= 0.0f) ||
                (enemy.direction < 0 && dx <= 0.0f);

            const bool inRangeX = fabsf(dx) <= enemy.attackRangeX;
            const bool inRangeY = dy <= enemy.attackRangeY;

            if (inFront && inRangeX && inRangeY &&
                !player.isHurt && !player.shieldActive && !player.isDashing)
            {
                Player_ApplyDamage(player, enemy.damage);

                float knockDir = (player.pos.x >= enemy.pos.x) ? 1.0f : -1.0f;
                player.vel.x = knockDir * 500.0f;
                player.knockbackTimer = player.hurtTimer > 0.0f ? player.hurtTimer : 0.6f;
                Camera_AddTrauma(0.5f);
            }
            enemy.hasAppliedAttackDamage = true;
            enemy.meleeCooldownTimer = enemy.meleeCooldown;
        }



        if (enemy.stateTimer <= 0.0f)
        {
            if (enemy.returnToHomeOnly)
                Enemy_SetState(enemy, EnemyState::Idle);
            else
                Enemy_SetState(enemy, EnemyState::Patrol);
        }
        break;
    }
    }

    if (enemy.knockbackTimer <= 0.0f && enemy.state != EnemyState::Patrol)
        enemy.vel.x = 0.0f;

    // Integrate Y normally
    enemy.pos.y += enemy.vel.y * dt;

    // Integrate X, but clamp to patrol/platform bounds
    const float halfWidth = enemy.width * 0.5f;
    const float minCenterX = enemy.patrolMinX + halfWidth;
    const float maxCenterX = enemy.patrolMaxX - halfWidth;

    float nextX = enemy.pos.x + enemy.vel.x * dt;

    if (nextX <= minCenterX)
    {
        enemy.pos.x = minCenterX;
        enemy.vel.x = 0.0f;
        enemy.knockbackVel = { 0.0f, 0.0f };
        enemy.knockbackTimer = 0.0f;
    }
    else if (nextX >= maxCenterX)
    {
        enemy.pos.x = maxCenterX;
        enemy.vel.x = 0.0f;
        enemy.knockbackVel = { 0.0f, 0.0f };
        enemy.knockbackTimer = 0.0f;
    }
    else
    {
        enemy.pos.x = nextX;
    }

    // Ground collision
    float groundY = enemy.platformY + (40.0f * 0.5f) + (enemy.height * 0.5f);

    if (enemy.pos.y <= groundY)
    {
        enemy.pos.y = groundY;
        enemy.vel.y = 0.0f;
        enemy.isGrounded = true;
    }
    else
    {
        enemy.isGrounded = false;
    }
    enemy.spriteSheet->Update(dt);
}

// Update each boss laser's internal state machine
void BossLasers_Update(Enemy& enemy, const Player& player, float dt)
{
    // TODO: replace hardcoded range gate with proper boss phase / aggro system
    /*const float dy = player.pos.y - enemy.pos.y;
    for (int i = 0; i < (int)enemy.bossLasers.size(); ++i)
    {
        std::cout << "[LASER " << i << "] state=" << (int)enemy.bossLasers[i].state
            << " cooldown=" << enemy.bossLasers[i].cooldownTimer << "\n";
    }
    if (fabsf(dy) > 300.0f)
    {
        std::cout << "[LASER] BLOCKED by range gate, dy=" << dy << "\n";
        return;
    }*/

    for (BossLaser& laser : enemy.bossLasers)
    {
        switch (laser.state)
        {
        case BossLaserState::Inactive:
            laser.active = false;
            laser.cooldownTimer -= dt;
            if (laser.cooldownTimer <= 0.0f) {
                laser.state = BossLaserState::Tracking;
                laser.stateTimer = enemy.laserTrackDuration;
            }
            break;

        case BossLaserState::Tracking:
            laser.target = player.pos;   // update every frame
            laser.stateTimer -= dt;
            if (laser.stateTimer <= 0.0f) {
                // target is now frozen — do not update it again
                laser.state = BossLaserState::LockOn;
                laser.stateTimer = enemy.laserLockDuration;
            }
            break;

        case BossLaserState::LockOn:
            // target stays frozen, just count down
            laser.stateTimer -= dt;
            if (laser.stateTimer <= 0.0f) {
                laser.active = true;
                laser.state = BossLaserState::Firing;
                laser.stateTimer = enemy.laserFireDuration;
            }
            break;

        case BossLaserState::Firing:
            laser.stateTimer -= dt;
            if (laser.stateTimer <= 0.0f) {
                laser.active = false;
                laser.state = BossLaserState::Inactive;
                laser.cooldownTimer = enemy.laserCooldown;
            }
            break;
        }
    }
}

void BossLasers_Draw(const Enemy& enemy)
{
    MeshManager& mm = MeshManager::Get();

    for (const BossLaser& laser : enemy.bossLasers)
    {
        switch (laser.state)
        {
        case BossLaserState::Tracking:
            // Thin yellow preview line
            mm.DrawLine(laser.origin.x, laser.origin.y,
                laser.target.x, laser.target.y,
                4.0f, 255, 255, 0, 0.5f);
            break;

        case BossLaserState::LockOn:
            // Thicker red telegraph line
            mm.DrawLine(laser.origin.x, laser.origin.y,
                laser.target.x, laser.target.y,
                8.0f, 255, 50, 50, 0.85f);
            break;

        case BossLaserState::Firing:
            // Full textured laser beam
            mm.DrawTexturedLine(enemy.laserTex,
                laser.origin.x, laser.origin.y,
                laser.target.x, laser.target.y,
                laser.width,
                64.0f,   // tile length in world units
                1.0f);
            break;

        default:
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Update boss enemy each frame
// Patrols left and right within a wider range than regular enemies
// Freezes briefly on hit stun
// -----------------------------------------------------------------------------
void BossEnemy_Update(Enemy& enemy, const Player& player, float dt) {

    (void)player;
    enemy.justDied = false;

    // drain hitStunTimer so hitJustStarted fires correctly on each new hit
    if (enemy.hitStunTimer > 0.0f)
    {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer < 0.0f) enemy.hitStunTimer = 0.0f;
    }

    // BossAI_Update owns all animation ticking — do nothing here
}

// -----------------------------------------------------------------------------
// called when player collides with enemy
// reduces hitPoints and sets hit stun
// -----------------------------------------------------------------------------
void Enemy_OnHit(Enemy& enemy, float damage, float knockbackDir, Player* attacker)
{
    if (!enemy.isAlive) return;

    bool killingBlow = (enemy.type == EnemyType::Boss && enemy.hitPoints - damage <= 0.0f);
    if (enemy.isInvincible && !killingBlow)
    {
        if (enemy.spriteSheet)
        {
            const std::string clip = enemy.spriteSheet->GetCurrentClip();
            if (clip == "hit")
            {
                Enemy_SetState(enemy, EnemyState::Patrol);
            }
        }
        return;
    }

    if (enemy.spriteSheet)
    {
        const std::string clip = enemy.spriteSheet->GetCurrentClip();

        if (enemy.type == EnemyType::Boss &&
            clip == "hurt")
        {
            return; // only boss blocks spam by animation
        }
    }

    if (enemy.type == EnemyType::Easy || enemy.type == EnemyType::Hard)
    {
        if (enemy.hitCooldownTimer > 0.0f)
            return;

        enemy.hitCooldownTimer = enemy.hitCooldown;
    }

    if (enemy.type == EnemyType::Boss && enemy.spriteSheet
        && enemy.spriteSheet->GetCurrentClip() == "block")
    {
        if (attacker)
        {
            float blockDir = (attacker->pos.x < enemy.pos.x) ? -1.0f : 1.0f;
            attacker->vel.x = blockDir * 1400.0f;
            attacker->vel.y = 200.0f;
            attacker->knockbackTimer = 0.8f;
            attacker->isHurt = true;
            attacker->hurtTimer = 0.8f;
        }
        return;
    }

    /*
    bool killingBlow = (enemy.type == EnemyType::Boss && enemy.hitPoints - damage <= 0.0f);
    if (enemy.isInvincible && !killingBlow) return;
    */
    // Only block damage if already in dead animation (allow knockback to re-trigger)
    if (enemy.spriteSheet)
    {
        const std::string clip = enemy.spriteSheet->GetCurrentClip();
        if (clip == "dead")
            return;
        /*
        // boss uses "hurt", regular enemies use "hit"
        const std::string hitClipName = (enemy.type == EnemyType::Boss) ? "hurt" : "hit";
        if (clip == hitClipName)
        {
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration(hitClipName);
            if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.5f;
            // boss does not receive knockback from player hits
            if (enemy.type != EnemyType::Boss)
                Enemy_ApplyKnockback(enemy, knockbackDir);
            return;
        }*/

        // boss uses "hurt", regular enemies use "hit"
        const std::string hitClipName = (enemy.type == EnemyType::Boss) ? "hurt" : "hit";

        // Easy/Hard: if already in hit animation, ignore extra hits completely
        if ((enemy.type == EnemyType::Easy || enemy.type == EnemyType::Hard) &&
            clip == hitClipName)
        {
            return;
        }
    }

    enemy.hitPoints -= damage;

    if (enemy.type == EnemyType::Easy)
    {
        AudioManager::Get().PlayAudio(s_EasyDamage, false);
    }
    else if (enemy.type == EnemyType::Hard)
    {
        AudioManager::Get().PlayAudio(s_HardDamage, false);
    }
    else if (enemy.type == EnemyType::Boss)
    {
        AudioManager::Get().PlayAudio(s_BossDamage, false);
    }

    // dead
    if (enemy.hitPoints <= 0.0f)
    {
        enemy.hitPoints = 0.0f;

        // boss death is handled by BossAI — don't set isAlive=false or play dead here
        if (enemy.type == EnemyType::Boss)
        {
            enemy.hitStunTimer = 0.3f; // brief stun so BossAI detects the hit
            return;
        }

        if (enemy.spriteSheet)
        {
            enemy.spriteSheet->Play("dead", true);
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("dead");
        }
        else enemy.hitStunTimer = 0.45f;


        enemy.justDied = true;
        enemy.isAlive = false;
        enemy.state = EnemyState::Idle;
        enemy.stateTimer = 0.0f;
        Enemy_OnDeath(enemy);
    }
    else
    {
        // hit
        if (enemy.spriteSheet)
        {
            const char* hitClip = (enemy.type == EnemyType::Boss) ? "hurt" : "hit";
            enemy.spriteSheet->Play(hitClip, true);
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration(hitClip);
            if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.5f;
            enemy.stateTimer = 0.0f;
        }
        else
        {
            enemy.hitStunTimer = 0.45f;
        }
    }

    // boss does not receive knockback from player hits
    if (enemy.type != EnemyType::Boss)
        Enemy_ApplyKnockback(enemy, knockbackDir);
}

void Enemy_OnDeath(Enemy& enemy)
{
    //// for debugging (to be removed)
    //ObjectManager::Get().SpawnBuff(BuffType::SHIELD, enemy.pos.x, enemy.pos.y);
    //ObjectManager::Get().SpawnBuff(BuffType::FULL_HP, enemy.pos.x + 50.f, enemy.pos.y);
    //ObjectManager::Get().SpawnBuff(BuffType::DASH , enemy.pos.x - 50.f, enemy.pos.y);
    if (enemy.type == EnemyType::Boss) return;
    BuffType droppedBuff = static_cast<BuffType>(rand() % 3 + 1); // random buff on drop
    ObjectManager::Get().SpawnBuff(droppedBuff, enemy.pos.x, enemy.pos.y);
}

void HardEnemy_OnCollision(Enemy& enemy, Player& player)
{
    if (!enemy.isAlive) return;
    if (!enemy.spriteSheet) return;

    const std::string clip = enemy.spriteSheet->GetCurrentClip();

    if (enemy.type != EnemyType::Boss)
    {
        if (enemy.state == EnemyState::Attack || clip == "hit" || clip == "dead")
            return;
    }

    // physically push player out of boss overlap every frame
    if (enemy.type == EnemyType::Boss)
    {
        float overlapDir = (player.pos.x >= enemy.pos.x) ? 1.0f : -1.0f;
        float combinedHalfW = (player.width + enemy.width) * 0.5f;
        float dist = fabsf(player.pos.x - enemy.pos.x);
        if (dist < combinedHalfW)
        {
            player.pos.x = enemy.pos.x + overlapDir * (combinedHalfW + 2.0f);
        }
    }

    // boss block check
    if (enemy.type == EnemyType::Boss && clip == "block")
    {
        if (!player.isHurt)
        {
            float blockDir = (player.pos.x < enemy.pos.x) ? -1.0f : 1.0f;
            player.vel.x = blockDir * 1400.0f;
            player.vel.y = 200.0f;
            player.knockbackTimer = 0.8f;
            player.isHurt = true;
            player.hurtTimer = 0.8f;
        }
        return;
    }

    // damage + knockback
    /*
    if (!player.isHurt && !player.shieldActive && !player.isDashing)
    {
        Player_ApplyDamage(player, enemy.damage);

        float knockDir = (player.pos.x >= enemy.pos.x) ? 1.0f : -1.0f;
        float knockSpeed = 500.0f;
        if (enemy.type == EnemyType::Boss)
        {
            if (clip == "slamimpact" || clip == "slamdown") knockSpeed = 900.0f;
            else if (clip == "attackpunch")                  knockSpeed = 700.0f;
        }
        player.vel.x = knockDir * knockSpeed;
        player.knockbackTimer = player.hurtTimer > 0.0f ? player.hurtTimer : 0.6f;
        Camera_AddTrauma(0.5f);
    }
    */
    // damage + knockback
    if (!player.isHurt && !player.shieldActive && !player.isDashing)
    {
        bool canDamage = false;
        float knockSpeed = 500.0f;

        if (enemy.type == EnemyType::Hard)
        {
            canDamage = false; // hard enemy damage is handled in HardEnemy_Update attack timing
        }
        else if (enemy.type == EnemyType::Boss)
        {
            if (clip == "slamimpact" || clip == "slamdown")
            {
                canDamage = true;
                knockSpeed = 900.0f;

                if (!enemy.hasAppliedAttackDamage && Enemy_IsInCamera(enemy))
                    AudioManager::Get().PlayAudio(s_BossAttackSound, false);
            }
            else if (clip == "attackpunch")
            {
                canDamage = true;
                knockSpeed = 700.0f;

                if (!enemy.hasAppliedAttackDamage && Enemy_IsInCamera(enemy))
                    AudioManager::Get().PlayAudio(s_BossAttackSound, false);
            }
        }

        if (canDamage)
        {
            Player_ApplyDamage(player, enemy.damage);

            float knockDir = (player.pos.x >= enemy.pos.x) ? 1.0f : -1.0f;
            player.vel.x = knockDir * knockSpeed;
            player.knockbackTimer = player.hurtTimer > 0.0f ? player.hurtTimer : 0.6f;
            Camera_AddTrauma(0.5f);
        }
    }
    /*
    if (enemy.type != EnemyType::Boss)
    {
        Enemy_SetState(enemy, EnemyState::Attack);
        AudioManager::Get().PlayAudio(s_HardEnemyAttackSound, false);
    }
    */

    if (enemy.type == EnemyType::Hard)
    {
        if (enemy.meleeCooldownTimer <= 0.0f &&
            enemy.state != EnemyState::Attack)
        {
            enemy.direction = (player.pos.x >= enemy.pos.x) ? 1 : -1;
            Enemy_SetState(enemy, EnemyState::Attack);

            AudioManager::Get().PlayAudio(s_HardEnemyAttackSound, false);
        }
    }
    // clamp player to arena bounds — prevent knockback sending them offscreen
    const float ARENA_MIN_X = -750.0f;
    const float ARENA_MAX_X = 750.0f;
    if (player.pos.x < ARENA_MIN_X) player.pos.x = ARENA_MIN_X;
    if (player.pos.x > ARENA_MAX_X) player.pos.x = ARENA_MAX_X;
}

// -----------------------------------------------------------------------------
// Free static resources (mesh and texture) remove after spritesheetd
// -----------------------------------------------------------------------------

void Enemy_ApplyKnockback(Enemy& enemy, float knockbackDir)
{
    // knockbackDir: -1 = knock left, 1 = knock right, 0 = no knockback
    if (knockbackDir == 0.0f)
        return;

    enemy.knockbackVel.x = knockbackDir * enemy.knockbackVelocity;
    enemy.knockbackVel.y = 0.0f;
    enemy.knockbackTimer = 0.3f;
}

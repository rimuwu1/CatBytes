/* Start Header ************************************************************************/
/*!
\file enemy.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
        kerwinjiajie.wong@digipen.edu
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
#include "AudioManager.h"
#include "Audio.h"
#include "Camera.h"
#include "SpriteSheet.h"
#include <fstream>
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"


static AEAudio s_EasyEnemyAttackSound{};
static AEAudio s_HardEnemyAttackSound{};
static AEAudio s_BossAttackSound{};

static bool Enemy_IsInCamera(const Enemy& enemy)
{
    const float halfScreenW = 1600.0f * 0.5f; // adjust if your game width is different
    const float halfScreenH = 900.0f * 0.5f;  // from your camera code

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

// -----------------------------------------------------------------------------
// initialize easy enemy
// sets position, size, direction, alive status, loads speed and textures
// -----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, const rapidjson::Value& config) {
    enemy.facesLeft = true;

    s_EasyEnemyAttackSound = AudioManager::Get().LoadAudio(Audio::EASY_ENEMY_ATTACK);

    // Position (required, but provide fallback)
    if (config.HasMember("x") && config["x"].IsFloat())
        enemy.pos.x = config["x"].GetFloat();
    else {
        enemy.pos.x = 0.0f;
        printf("Warning: Enemy missing 'x', defaulting to 0\n");
    }
    if (config.HasMember("y") && config["y"].IsFloat())
        enemy.pos.y = config["y"].GetFloat();
    else {
        enemy.pos.y = 0.0f;
        printf("Warning: Enemy missing 'y', defaulting to 0\n");
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

    if (config.HasMember("bullet_height") && config["bullet_height"].IsFloat())
        enemy.bulletHeight = config["bullet_height"].GetFloat();

    enemy.shootTimer = enemy.shootCooldown;
    enemy.vel = { 0.0f, 0.0f };
    enemy.direction = 1;

    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Easy;

    // Patrol bounds
    if (config.HasMember("patrol_min_x") && config["patrol_min_x"].IsFloat())
        enemy.patrolMinX = config["patrol_min_x"].GetFloat();
    else
        enemy.patrolMinX = enemy.pos.x - 100.0f;

    if (config.HasMember("patrol_max_x") && config["patrol_max_x"].IsFloat())
        enemy.patrolMaxX = config["patrol_max_x"].GetFloat();
    else
        enemy.patrolMaxX = enemy.pos.x + 100.0f;

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
}

void HardEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {

    s_HardEnemyAttackSound = AudioManager::Get().LoadAudio(Audio::HARD_ENEMY_ATTACK);

    // Position
    if (config.HasMember("x") && config["x"].IsFloat())
        enemy.pos.x = config["x"].GetFloat();
    else {
        enemy.pos.x = 0.0f;
        printf("Warning: HardEnemy missing 'x', defaulting to 0\n");
    }
    if (config.HasMember("y") && config["y"].IsFloat())
        enemy.pos.y = config["y"].GetFloat();
    else {
        enemy.pos.y = 0.0f;
        printf("Warning: HardEnemy missing 'y', defaulting to 0\n");
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

    enemy.shootCooldown = 0.0f; // no shooting
    enemy.vel = { 0.0f, 0.0f };
    enemy.direction = 1;
    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Hard;

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
}

void BossEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {

    enemy.facesLeft = true;
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
    enemy.direction = 1;
    if (enemy.hitPoints <= 0.0f) {
        enemy.hitPoints = 0.0f;
        enemy.isAlive = false;
    }
    else {
        enemy.isAlive = true;
    }
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Boss;

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

        enemy.spriteSheet->Play("patrol");
    }
}

// -----------------------------------------------------------------------------
// Update enemy: automatic left/right patrol
// -----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt) {

    if (!enemy.spriteSheet) return;

    const std::string currentClip = enemy.spriteSheet->GetCurrentClip();

    // keep dead animation running until it finishes, then stop drawing
    if (!enemy.isAlive)
    {
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
        // Apply knockback movement first
        if (enemy.knockbackTimer > 0.0f)
        {
            enemy.knockbackTimer -= dt;
            enemy.pos.x += enemy.knockbackVel.x * dt;
            if (enemy.knockbackTimer <= 0.0f)
            {
                enemy.knockbackTimer = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
            }
        }
        else
        {
            // Only decrement hitstun after knockback is done
            enemy.hitStunTimer -= dt;
        }

        enemy.spriteSheet->Update(dt);

        if (enemy.hitStunTimer <= 0.0f)
        {
            enemy.hitStunTimer = 0.0f;

            const std::string clipNow = enemy.spriteSheet->GetCurrentClip();
            if (clipNow != "dead")
                enemy.spriteSheet->Play("patrol");
        }
        return;
    }


    // Shooting (if applicable)
    if (enemy.shootCooldown > 0.0f) {
        enemy.shootTimer -= dt;
        if (enemy.shootTimer <= 0.0f) {
            enemy.spriteSheet->Play("attack", true);
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("attack");

            ObjectManager::Get().SpawnEnemyBullet(
                enemy,
                enemy.bulletSpeed,
                enemy.bulletDamage,
                enemy.bulletRange
            );

            //only play audio if enemy is in the camera view
            if (Enemy_IsInCamera(enemy))
            {
                AudioManager::Get().PlayAudio(s_EasyEnemyAttackSound, false);
            }

            enemy.shootTimer = enemy.shootCooldown;
            return;
        }
    }

    enemy.vel.x = enemy.direction * enemy.moveSpeed;
    PhysicsManager::Get().Integrate(enemy.pos, enemy.vel, dt);

    
    // Patrol movement (TODO, now still hardcoded)
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;

    /*
    // Patrol bounds (could be from config, but keep hardcoded for now)
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
    */
    if (enemy.pos.x >= enemy.patrolMaxX) {
        enemy.pos.x = enemy.patrolMaxX;
        enemy.direction = -1;
    }
    else if (enemy.pos.x <= enemy.patrolMinX) {
        enemy.pos.x = enemy.patrolMinX;
        enemy.direction = 1;
    }

    // Update animation
        if (enemy.spriteSheet->GetCurrentClip() != "patrol")
            enemy.spriteSheet->Play("patrol");

        enemy.spriteSheet->Update(dt);
}

void HardEnemy_Update(Enemy& enemy, float dt) {
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
        // Apply knockback movement first
        if (enemy.knockbackTimer > 0.0f)
        {
            enemy.knockbackTimer -= dt;
            enemy.pos.x += enemy.knockbackVel.x * dt;
            if (enemy.knockbackTimer <= 0.0f)
            {
                enemy.knockbackTimer = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
            }
        }
        else
        {
            // Only decrement hitstun after knockback is done
            enemy.hitStunTimer -= dt;
        }

        enemy.spriteSheet->Update(dt);

        if (enemy.hitStunTimer <= 0.0f)
        {
            enemy.hitStunTimer = 0.0f;

            // Only go back to patrol from attack/hit
            const std::string clipNow = enemy.spriteSheet->GetCurrentClip();
            if (clipNow != "dead")
                enemy.spriteSheet->Play("patrol");
        }
        return;
    }

    enemy.vel.x = enemy.direction * enemy.moveSpeed;
    PhysicsManager::Get().Integrate(enemy.pos, enemy.vel, dt);
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;

    /*
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
    */

    if (enemy.pos.x >= enemy.patrolMaxX) {
        enemy.pos.x = enemy.patrolMaxX;
        enemy.direction = -1;
    }
    else if (enemy.pos.x <= enemy.patrolMinX) {
        enemy.pos.x = enemy.patrolMinX;
        enemy.direction = 1;
    }

    if (enemy.spriteSheet->GetCurrentClip() != "patrol")
        enemy.spriteSheet->Play("patrol");

    enemy.spriteSheet->Update(dt);
}

// -----------------------------------------------------------------------------
// Update boss enemy each frame
// Patrols left and right within a wider range than regular enemies
// Freezes briefly on hit stun
// -----------------------------------------------------------------------------
void BossEnemy_Update(Enemy& enemy, float dt) {

    if (!enemy.spriteSheet) return;

    const std::string currentClip = enemy.spriteSheet->GetCurrentClip();

    if (enemy.hitStunTimer > 0.0f) {
        // Apply knockback movement first
        if (enemy.knockbackTimer > 0.0f)
        {
            enemy.knockbackTimer -= dt;
            enemy.pos.x += enemy.knockbackVel.x * dt;
            if (enemy.knockbackTimer <= 0.0f)
            {
                enemy.knockbackTimer = 0.0f;
                enemy.knockbackVel = { 0.0f, 0.0f };
            }
        }
        else
        {
            // Only decrement hitstun after knockback is done
            enemy.hitStunTimer -= dt;
        }

        if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.0f;
        return;
    }
    enemy.vel.x = enemy.direction * enemy.moveSpeed;
    PhysicsManager::Get().Integrate(enemy.pos, enemy.vel, dt);
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
}

// -----------------------------------------------------------------------------
// Draw enemy on screen (deprecated, in obj manager now)
// -----------------------------------------------------------------------------
//void Enemy_Draw(const Enemy& enemy)
//{
//    //if (!enemy.isAlive && (!enemy.spriteSheet || enemy.spriteSheet->GetCurrentClip() != "dead"))
//      //  return;
//
//    if (!enemy.spriteSheet)
//        return;
//
//    // dead enemy should disappear after dead animation duration ends
//    if (!enemy.isAlive)
//    {
//        if (enemy.spriteSheet->GetCurrentClip() != "dead")
//            return;
//
//        if (enemy.hitStunTimer <= 0.0f)
//            return;
//    }
//
//    float scaleX;
//
//    if (enemy.facesLeft)
//        scaleX = (enemy.direction == 1) ? -enemy.width : enemy.width;
//    else
//        scaleX = (enemy.direction == -1) ? -enemy.width : enemy.width;
//
//    MeshManager::Get().DrawSpriteSheet(
//        *enemy.spriteSheet,
//        enemy.pos.x,
//        enemy.pos.y,
//        scaleX,
//        enemy.height,
//        1.0f
//    );
//
//    // enemy hp bar
//    if (enemy.isAlive && enemy.maxHitPoints > 0.0f) {
//        float hpRatio = enemy.hitPoints / enemy.maxHitPoints;
//
//        float hpBarWidth = enemy.width * 0.8f;
//        float hpBarHeight = 6.0f;
//
//        float x = enemy.pos.x;
//        float y = enemy.pos.y + enemy.height * 0.5f + 10.0f;
//
//        // outline
//        MeshManager::Get().DrawSquare(
//            x,
//            y,
//            hpBarWidth + 6.0f,
//            hpBarHeight + 6.0f,
//            0, 0, 0, 1.0f
//        );
//
//        // background
//        MeshManager::Get().DrawSquare(
//            x, 
//            y, 
//            hpBarWidth, 
//            hpBarHeight, 
//            40, 40, 40, 1.0f
//        );
//
//        // remaining health
//        MeshManager::Get().DrawSquare(
//            x - (hpBarWidth / 2) + (hpBarWidth * hpRatio) * 0.5f,
//            y, 
//            hpBarWidth * hpRatio, 
//            hpBarHeight, 
//            220, 40, 40, 1.0f
//        );
//    }
//
//}

// -----------------------------------------------------------------------------
// called when player collides with enemy
// reduces hitPoints and sets hit stun
// -----------------------------------------------------------------------------
void Enemy_OnHit(Enemy& enemy, float damage, float knockbackDir)
{
    if (!enemy.isAlive)
        return;

    // Only block damage if already in dead animation (allow knockback to re-trigger)
    if (enemy.spriteSheet)
    {
        const std::string clip = enemy.spriteSheet->GetCurrentClip();
        if (clip == "dead")
            return;
        
        // If already in hit animation, extend hitstun and apply knockback
        if (clip == "hit")
        {
            // Extend hitstun timer
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("hit");
            Enemy_ApplyKnockback(enemy, knockbackDir);
            return;
        }
    }

    enemy.hitPoints -= damage;

    // dead
    if (enemy.hitPoints <= 0.0f)
    {
        enemy.hitPoints = 0.0f;

        if (enemy.spriteSheet)
        {
            enemy.spriteSheet->Play("dead", true);
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("dead");
        }
        else
        {
            enemy.hitStunTimer = 0.45f;
        }

        enemy.isAlive = false;
    }
    else
    {
        // hit
        if (enemy.spriteSheet)
        {
            enemy.spriteSheet->Play("hit", true);
            enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("hit");
        }
        else
        {
            enemy.hitStunTimer = 0.45f;
        }
    }

    // Apply knockback
    Enemy_ApplyKnockback(enemy, knockbackDir);
}


void HardEnemy_OnCollision(Enemy& enemy, Player& player)
{
    if (!enemy.isAlive)
        return;

    if (!enemy.spriteSheet)
        return;

    // don't restart attack if already in attack/hit/dead
    const std::string clip = enemy.spriteSheet->GetCurrentClip();
    if (clip == "attack" || clip == "hit" || clip == "dead")
        return;

    // apply damage
    Player_ApplyDamage(player, enemy.damage);

    // apply knockback to player (away from enemy)
    Player_ApplyKnockback(player, enemy.pos.x, enemy.pos.y);

    // switch to attack animation
    enemy.spriteSheet->Play("attack", true);
    enemy.hitStunTimer = enemy.spriteSheet->GetClipTotalDuration("attack");

    AudioManager::Get().PlayAudio(s_HardEnemyAttackSound, false);
}


// -----------------------------------------------------------------------------
// Free static resources (mesh and texture) remove after spritesheet
// -----------------------------------------------------------------------------
//void Enemy_Free(Enemy& enemy)
//{
//}

void Enemy_ApplyKnockback(Enemy& enemy, float knockbackDir)
{
    // knockbackDir: -1 = knock left, 1 = knock right, 0 = no knockback
    if (knockbackDir == 0.0f)
        return;

    enemy.knockbackVel.x = knockbackDir * enemy.knockbackVelocity;
    enemy.knockbackVel.y = 0.0f;
    enemy.knockbackTimer = enemy.hitStunTimer;
}
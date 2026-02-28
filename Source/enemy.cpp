/* Start Header ************************************************************************/
/*!
\file enemy.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
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
#include "Player.h"
#include <fstream>
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"

extern rapidjson::Document configDoc;

// -----------------------------------------------------------------------------
// initialize easy enemy
// sets position, size, direction, alive status, loads speed and textures
// -----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, const rapidjson::Value& config) {
    enemy.facesLeft = true;

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
    if (config.HasMember("hp") && config["hp"].IsFloat())
        enemy.hitPoints = config["hp"].GetFloat();
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

    enemy.shootTimer = enemy.shootCooldown;
    enemy.direction = 1;
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Easy;
}

void HardEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {
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
    if (config.HasMember("hp") && config["hp"].IsFloat())
        enemy.hitPoints = config["hp"].GetFloat();
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
    enemy.direction = 1;
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Hard;
}

void BossEnemy_Init(Enemy& enemy, const rapidjson::Value& config) {
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
    if (config.HasMember("hp") && config["hp"].IsFloat())
        enemy.hitPoints = config["hp"].GetFloat();
    else {
        enemy.hitPoints = 30.0f;
        printf("Warning: Boss missing 'hp', defaulting to 30\n");
    }

    // Collision damage
    if (config.HasMember("damage") && config["damage"].IsFloat())
        enemy.damage = config["damage"].GetFloat();
    else {
        enemy.damage = 8.0f;
        printf("Warning: Boss missing 'damage', defaulting to 8\n");
    }

    enemy.shootCooldown = 0.0f;
    enemy.direction = 1;
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;
    enemy.type = EnemyType::Boss;
}

// -----------------------------------------------------------------------------
// Update enemy: automatic left/right patrol
// -----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt) {
    if (!enemy.isAlive) return;
    if (enemy.hitStunTimer > 0.0f) {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.0f;
        return;
    }

    // Shooting (if applicable)
    if (enemy.shootCooldown > 0.0f) {
        enemy.shootTimer -= dt;
        if (enemy.shootTimer <= 0.0f) {
            ObjectManager::Get().SpawnEnemyBullet(enemy, enemy.bulletSpeed, enemy.bulletDamage, enemy.bulletRange);
            enemy.shootTimer = enemy.shootCooldown;
        }
    }

    // Patrol movement
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;
    // Patrol bounds (could be from config, but keep hardcoded for now)
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
}

void HardEnemy_Update(Enemy& enemy, float dt) {
    if (!enemy.isAlive) return;
    if (enemy.hitStunTimer > 0.0f) {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer <= 0.0f) {
            enemy.hitStunTimer = 0.0f;
            enemy.texture = enemy.normalTexture;
        }
        return;
    }
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
}

// -----------------------------------------------------------------------------
// Update boss enemy each frame
// Patrols left and right within a wider range than regular enemies
// Freezes briefly on hit stun
// -----------------------------------------------------------------------------
void BossEnemy_Update(Enemy& enemy, float dt) {
    if (!enemy.isAlive) return;
    if (enemy.hitStunTimer > 0.0f) {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.0f;
        return;
    }
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;
    float patrolMinX = -400.0f, patrolMaxX = 400.0f;
    if (enemy.pos.x >= patrolMaxX) enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX) enemy.direction = 1;
}

// -----------------------------------------------------------------------------
// Draw enemy on screen
// -----------------------------------------------------------------------------
void Enemy_Draw(const Enemy& enemy)
{
    if (!enemy.isAlive) return;

    // Horizontal flip based on direction and which way the image faces
    float scaleX;
    if (enemy.facesLeft)
        scaleX = (enemy.direction == 1) ? -enemy.width : enemy.width;
    else
        scaleX = (enemy.direction == -1) ? -enemy.width : enemy.width;

    // Draw enemy body (if texture exists)
    if (enemy.texture) {
        MeshManager::Get().DrawTexturedSquare(
            enemy.texture,
            enemy.pos.x,
            enemy.pos.y,
            scaleX,
            enemy.height
        );
    }

    // Low?HP overlay
    float alpha = 0.0f;
    if (enemy.hitPoints == 1)
        alpha = 0.5f;
    else if (enemy.hitStunTimer > 0.0f)
        alpha = (enemy.hitStunTimer / 0.5f) * 1.0f;

    if (alpha > 0.0f && enemy.lowHpTexture) {
        MeshManager::Get().DrawTexturedSquare(
            enemy.lowHpTexture,
            enemy.pos.x,
            enemy.pos.y,
            enemy.width,
            enemy.height,
            alpha
        );
    }
}

// -----------------------------------------------------------------------------
// called when player collides with enemy
// reduces hitPoints and sets hit stun
// -----------------------------------------------------------------------------
void Enemy_OnHit(Enemy& enemy, float damage)
{
    if (!enemy.isAlive || enemy.hitStunTimer > 0.0f)
        return; // already hit, or dead

    // decrease hit points
    enemy.hitPoints -=damage;

    // freeze enemy for 0.5 seconds
    enemy.hitStunTimer = 0.5f;

    // if HP <= 0, enemy dies
    if (enemy.hitPoints <= 0)
        enemy.isAlive = 0;
}

void Enemy_SetGraphics(Enemy& enemy, AEGfxTexture* normalTex, AEGfxTexture* attackTex, AEGfxTexture* lowHpTex)
{
    
    enemy.texture = normalTex;
    enemy.normalTexture = normalTex;

    enemy.attackTexture = attackTex;
    enemy.lowHpTexture = lowHpTex;
}


void HardEnemy_OnCollision(Enemy& enemy, Player& player)
{
    if (!enemy.isAlive)
        return;

    // prevent repeated hits while already attacking
    if (enemy.texture == enemy.attackTexture)
        return;

    // apply damage
    Player_ApplyDamage(player, enemy.damage);

    // switch to attack state
    enemy.hitStunTimer = 0.3f;
    enemy.texture = enemy.attackTexture;
}


// -----------------------------------------------------------------------------
// Free static resources (mesh and texture) remove after spritesheet
// -----------------------------------------------------------------------------
void Enemy_Free(Enemy& enemy)
{
    // Textures are owned globally (e.g., in Level1_Unload).
    // Here we just nullify pointers to avoid stale references.
    enemy.texture = nullptr;
    enemy.normalTexture = nullptr;
    enemy.attackTexture = nullptr;
    enemy.lowHpTexture = nullptr;
}
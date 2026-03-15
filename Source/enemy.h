/* Start Header ************************************************************************/
/*!
\file enemy.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
        kerwinjiajie.wong@digipen.edu
\date Junuary, 24, 2026
\brief the Enemy struct stores position, size, movement speed, direction, and alive status. 
Functions initialize the enemy, update its position, draw it on screen, and free static resources

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include "rapidjson/document.h"
#include "SpriteSheet.h"
#include <memory>

struct Player;

enum class EnemyType {
    Easy,
    Hard,
    Boss
};

enum class EnemyState {
    Idle,
    Patrol,
    Attack
};

struct Enemy
{
    EnemyType type = EnemyType::Easy;   // which kind of enemy
    bool facesLeft = false;

    std::unique_ptr<SpriteSheet> spriteSheet;
    //std::string currentState = "patrol";
    EnemyState state = EnemyState::Patrol;
    float stateTimer = 0.0f;   //generic timer for idle/attack state
    float idleDuration = 1.0f; //how long enemy stays idle

    AEVec2 pos = { 0.0f, 0.0f };
    AEVec2 vel = { 0.0f, 0.0f };
    float width = 0.0f;
    float height = 0.0f;

    float moveSpeed = 0.0f;
    int direction = 1;
    int isAlive = 0;

    float hitPoints = 0.0f;
    float maxHitPoints = 0.0f;
    float hitStunTimer = 0.0f;
    bool isPlayerColliding = false;

    float patrolMinX = 0.0f;
    float patrolMaxX = 0.0f;

    // Shooting (for enemies that shoot)
    float shootCooldown = 0.0f;
    float shootTimer = 0.0f;
    float bulletSpeed = 0.0f;
    float bulletDamage = 0.0f;
    float bulletRange = 0.0f;
    float bulletWidth = 30.0f;
    float bulletHeight = 30.0f;
    std::unique_ptr<SpriteSheet> bulletSprite;

    // HardEnemy only
    float damage = 0.0f;

    // knockback state
    AEVec2 knockbackVel{ 0.0f, 0.0f };
    float knockbackTimer = 0.0f;
    float knockbackVelocity = 300.0f;
};

// Initialisation functions take a config object
void Enemy_Init(Enemy& enemy, const rapidjson::Value& config);
void HardEnemy_Init(Enemy& enemy, const rapidjson::Value& config);
void BossEnemy_Init(Enemy& enemy, const rapidjson::Value& config);

// Update functions (call ObjectManager to spawn bullets)
void Enemy_Update(Enemy& enemy, float dt);
void HardEnemy_Update(Enemy& enemy, float dt);
void BossEnemy_Update(Enemy& enemy, float dt);

// Draw, onHit, setGraphics, free
void Enemy_Draw(const Enemy& enemy);
void Enemy_OnHit(Enemy& enemy, float damage, float knockbackDir = 0.0f);
void Enemy_OnDeath(Enemy& enemy);
//void Enemy_Free(Enemy& enemy);
void HardEnemy_OnCollision(Enemy& enemy, Player& player);
void Enemy_ApplyKnockback(Enemy& enemy, float knockbackDir);
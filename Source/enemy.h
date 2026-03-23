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
#include <vector>

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

// Boss laser states
enum class BossLaserState {
    Inactive,   // waiting for cooldown before next attack
    Tracking,   // preview line tracks player position each frame
    LockOn,     // target frozen, telegraph held before firing
    Firing,     // laser active, dealing damage
};

struct BossLaser {
    AEVec2 origin  = { 0.0f, 0.0f };  // fixed world position, set from config
    AEVec2 target  = { 0.0f, 0.0f };  // locked-on player position (frozen at LockOn)
    float  width   = 16.0f;            // hitbox thickness
    bool   active  = false;            // true only during Firing state

    BossLaserState state       = BossLaserState::Inactive;
    float          stateTimer  = 0.0f;  // counts down current phase
    float          cooldownTimer = 0.0f; // counts down before next Tracking phase
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
    int homeDirection = 1;
    bool isAlive = false;
    bool justDied = false;

    float hitPoints = 0.0f;
    float maxHitPoints = 0.0f;
    float hitStunTimer = 0.0f;
    bool isPlayerColliding = false;

    float patrolMinX = 0.0f;
    float patrolMaxX = 0.0f;

    bool returnToHomeOnly = false;
    float homeX = 0.0f;

    float platformY = 0.0f;

    // Shooting (for enemies that shoot)
    float shootCooldown = 0.0f;
    float shootTimer = 0.0f;
    float bulletSpeed = 0.0f;
    float bulletDamage = 0.0f;
    float bulletRange = 0.0f;
    float bulletWidth = 30.0f;
    float bulletHeight = 30.0f;
    std::unique_ptr<SpriteSheet> bulletSprite;

    bool isGrounded = false;

    float damage = 0.0f;

    // knockback state
    AEVec2 knockbackVel{ 0.0f, 0.0f };
    float knockbackTimer = 0.0f;
    float knockbackVelocity = 300.0f;
    
    // Boss laser attack
    std::vector<BossLaser> bossLasers;          // one entry per emitter, loaded from config
    float laserCooldown      = 3.0f;            // shared: time between attacks
    float laserTrackDuration = 1.5f;            // shared: tracking phase length
    float laserLockDuration  = 0.8f;            // shared: lock-on telegraph length
    float laserFireDuration  = 0.6f;            // shared: firing phase length
    float laserDamage        = 10.0f;           // damage per hit
    float laserKnockback     = 400.0f;          // knockback force on hit
    AEGfxTexture* laserTex = nullptr;

    // bossAI
    bool isInvincible = false;  // blocks damage during phase changes
};

// Initialisation functions take a config object
void Enemy_Init(Enemy& enemy, const rapidjson::Value& config);
void HardEnemy_Init(Enemy& enemy, const rapidjson::Value& config);
void BossEnemy_Init(Enemy& enemy, const rapidjson::Value& config);

// Update functions (call ObjectManager to spawn bullets)
void Enemy_Update(Enemy& enemy, float dt);
void HardEnemy_Update(Enemy& enemy, float dt);
void BossEnemy_Update(Enemy& enemy, const Player& player, float dt); // TODO: update BossEnemy_Update call site in ObjectManager.cpp to pass the Player

// Boss laser update (updates each laser's internal state)
void BossLasers_Update(Enemy& enemy, const Player& player, float dt);
void BossLasers_Draw(const Enemy& enemy);

void Enemy_OnHit(Enemy& enemy, float damage, float knockbackDir = 0.0f);
void Enemy_OnDeath(Enemy& enemy);
void HardEnemy_OnCollision(Enemy& enemy, Player& player);
void Enemy_ApplyKnockback(Enemy& enemy, float knockbackDir);

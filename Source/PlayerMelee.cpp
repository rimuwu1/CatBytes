/* Start Header ************************************************************************/
/*!
\file PlayerMelee.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief Implements functions for the player's melee attacks, including attack updates,
       hitbox calculations, collision detection with enemies, and applying damage.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "PlayerMelee.h"
#include <cmath>
#include <vector> 

void PlayerMelee_Init(Player & player)
{
    //reset attack state flags
    player.isAttacking = false;
    player.attackTimer = 0.0f;
    player.meleeHasHitThisSwing = false;
}

void PlayerMelee_Update(Player& player, Enemy& enemy)
{
    //only proceed if player is attacking, weapon equipped, and enemy alive
    if (player.weapon != PlayerWeapon::MELEE || !player.weaponEquipped || !player.isAttacking || !enemy.isAlive)
        return;

    //prevent hitting the same enemy multiple times in a single swing
    if (player.meleeHasHitThisSwing)
        return;

    //define melee hitbox dimensions(rn its just player+weapon length)
    const float weaponWidth = 30.0f;
    const float weaponHeight = 80.0f;
    const float offsetX = player.width * 0.5f + 40.0f; //position hitbox in front of player

    //calculate hitbox center position based on player facing direction
    float weaponX = player.pos.x + (player.facingRight ? offsetX : -offsetX);
    float weaponY = player.pos.y;

    //Half-dimensions for overlap check
    float weaponHalfW = weaponWidth * 0.5f;
    float weaponHalfH = weaponHeight * 0.5f;
    float enemyHalfW = enemy.width * 0.5f;
    float enemyHalfH = enemy.height * 0.5f;

    //check AABB collision(axis-aligned bounding box)
    bool overlapX = fabs(weaponX - enemy.pos.x) < (weaponHalfW + enemyHalfW);
    bool overlapY = fabs(weaponY - enemy.pos.y) < (weaponHalfH + enemyHalfH);

    //If hitbox overlaps enemy, apply damage
    if (overlapX && overlapY)
    {
        Enemy_OnHit(enemy, player.meleeDamage);
        player.meleeHasHitThisSwing = true;
    }
}

void PlayerMelee_CheckCollisions(Player& player, std::vector<Enemy*>& enemies)
{
    for (Enemy* enemy : enemies)
    {
        PlayerMelee_Update(player, *enemy);
    }
}

void PlayerMelee_CheckBossCollision(Player& player, Enemy& boss)
{
    //only proceed if attacking, weapon equipped, and boss alive
    if (player.weapon != PlayerWeapon::MELEE || !player.weaponEquipped || !player.isAttacking || !boss.isAlive)
        return;

    //prevent hitting the boss multiple times in one swing
    if (player.meleeHasHitThisSwing)
        return;

    //define hitbox dimensions
    const float weaponWidth = 30.0f;
    const float weaponHeight = 80.0f;
    const float offsetX = player.width * 0.5f + 40.0f;

    // compute hitbox center
    float weaponX = player.pos.x + (player.facingRight ? offsetX : -offsetX);
    float weaponY = player.pos.y;

    float weaponHalfW = weaponWidth * 0.5f;
    float weaponHalfH = weaponHeight * 0.5f;
    float bossHalfW = boss.width * 0.5f;
    float bossHalfH = boss.height * 0.5f;

    bool overlapX = fabs(weaponX - boss.pos.x) < (weaponHalfW + bossHalfW);
    bool overlapY = fabs(weaponY - boss.pos.y) < (weaponHalfH + bossHalfH);

    //apply damage if collision occurs
    if (overlapX && overlapY)
    {
        Enemy_OnHit(boss, player.meleeDamage); //boss is treated as an Enemy
        player.meleeHasHitThisSwing = true;
    }
}

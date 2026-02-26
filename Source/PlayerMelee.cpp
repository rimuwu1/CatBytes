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
#include "MeshManager.h"
#include <cmath>
#include <vector>

// ----------------------------------------------------------------------------
// Returns the world-space AABB of the slash sprite as it is drawn.
// Matches exactly the position and size used in Player_Draw.
// Returns false if the slash is not active or the sprite doesn't exist.
// ----------------------------------------------------------------------------
static bool GetSlashAABB(const Player& player, float& outX, float& outY, float& outHalfW, float& outHalfH)
{
    if (!player.isAttacking || !player.slashSprite)
        return false;

    const float slashWidth = player.width;      // match player size for slash hitbox
    const float slashHeight = player.height;
    const float offset = 20.0f;          // distance from player edge

    outHalfW = slashWidth * 0.5f;
    outHalfH = slashHeight * 0.5f;

    switch (player.slashDirection) {
    case SlashDirection::HORIZONTAL:
        outX = player.pos.x + (player.facingRight ? player.width * 0.5f + offset : -player.width * 0.5f - offset);
        outY = player.pos.y;
        break;
    case SlashDirection::UP:
        outX = player.pos.x;
        outY = player.pos.y + player.height * 0.5f + offset;
        break;
    case SlashDirection::DOWN:
        outX = player.pos.x;
        outY = player.pos.y - player.height * 0.5f - offset;
        break;
    default:
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// AABB vs AABB overlap test using centre + half-extents.
// ----------------------------------------------------------------------------
static bool AABBOverlap(float ax, float ay, float aHW, float aHH,
    float bx, float by, float bHW, float bHH)
{
    return (fabs(ax - bx) < (aHW + bHW)) &&
        (fabs(ay - by) < (aHH + bHH));
}

// ----------------------------------------------------------------------------
void PlayerMelee_Init(Player& player)
{
    player.isAttacking = false;
    player.attackTimer = 0.0f;
}

// ----------------------------------------------------------------------------
// Collision check against a single enemy.
// Uses the slash sprite's drawn AABB — if the sprite is visible and overlaps
// the enemy, damage is applied once per swing.
// ----------------------------------------------------------------------------
void PlayerMelee_Update(Player& player, Enemy& enemy)
{
    if (!enemy.isAlive)
        return;


    float slashX, slashY, slashHW, slashHH;
    if (!GetSlashAABB(player, slashX, slashY, slashHW, slashHH))
        return;

    float enemyHW = enemy.width * 0.5f;
    float enemyHH = enemy.height * 0.5f;

    if (AABBOverlap(slashX, slashY, slashHW, slashHH,
        enemy.pos.x, enemy.pos.y, enemyHW, enemyHH))
    {
        Enemy_OnHit(enemy, player.meleeDamage);
    }
}

// ----------------------------------------------------------------------------
// Collision check against a list of enemies.
// Note: meleeHasHitThisSwing is shared (remove the flag check above
// and track hit enemies individually instead).
// ----------------------------------------------------------------------------
void PlayerMelee_CheckCollisions(Player& player, std::vector<Enemy*>& enemies)
{
    float slashX, slashY, slashHW, slashHH;
    if (!GetSlashAABB(player, slashX, slashY, slashHW, slashHH))
        return;

    for (Enemy* enemy : enemies)
    {
        if (!enemy || !enemy->isAlive) continue;

        float enemyHW = enemy->width * 0.5f;
        float enemyHH = enemy->height * 0.5f;

        if (AABBOverlap(slashX, slashY, slashHW, slashHH,
            enemy->pos.x, enemy->pos.y, enemyHW, enemyHH))
        {
            Enemy_OnHit(*enemy, player.meleeDamage);

            // Down?slash jump: only once per swing
            if (player.slashDirection == SlashDirection::DOWN && !player.downSlashJumped)
            {
                const float JUMP_FORCE = 650.0f;   // same as in Input.cpp
                player.vel.y = JUMP_FORCE;
                player.grounded = false;
                player.downSlashJumped = true;
            }
        }
    }
}

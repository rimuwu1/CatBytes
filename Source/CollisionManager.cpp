/* Start Header ************************************************************************/
/*!
\file       CollisionManager.cpp
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date       Feb 26 2026
\brief		This file handles all the collision checks under the collsion namespace.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "CollisionManager.h"
#include "Platforms.h"          // for Platform_CollisionCheck, WallCollisionCheck, CheckObstacleCollision, CheckpointCollisionCheck
#include "Player.h"             // for Player_ApplyDamage
#include "PlayerBullet.h"       // for Player_CheckBulletCollisions
#include "PlayerMelee.h"        // for PlayerMelee_CheckCollisions
#include <cmath>

namespace CollisionManager
{
    // ------------------------------------------------------------------------
    // Ground
    void HandleGround(Player& player, float groundY, float groundHeight, float& playerPrevY)
    {
        float groundTop = groundY + groundHeight * 0.5f;
        if (player.pos.y - player.height * 0.5f <= groundTop)
        {
            player.pos.y = groundTop + player.height * 0.5f;
            player.vel.y = 0.0f;
            player.grounded = 1;
            playerPrevY = player.pos.y;
        }
    }

    // ------------------------------------------------------------------------
    // Platforms (standard check, usually called only when falling)
    void HandlePlatforms(Player& player, float playerPrevY, const std::vector<Platform>& platforms)
    {
        if (player.vel.y <= 0.0f)
        {
            // Now properly declared via Platforms.h
            Platform_CollisionCheck(player, playerPrevY, platforms);
        }
    }

    // ------------------------------------------------------------------------
    // Landing on any platform from multiple sets (combined correction)
    bool HandleLandingOnAnyPlatform(Player& player, float playerPrevY,
        const std::vector<Platform>& platforms1,
        const std::vector<Platform>& platforms2,
        const std::vector<Platform>& platforms3,
        const std::vector<Platform>& platforms4)
    {
        float playerPrevBottom = playerPrevY - player.height * 0.5f;
        float playerCurrBottom = player.pos.y - player.height * 0.5f;

        auto checkSet = [&](const std::vector<Platform>& platforms) -> bool
            {
                for (const Platform& pf : platforms)
                {
                    if (!pf.active) continue;
                    float pfLeft = pf.x - pf.w * 0.5f;
                    float pfRight = pf.x + pf.w * 0.5f;
                    float pfTop = pf.y + pf.h * 0.5f;
                    float playerLeft = player.pos.x - player.width * 0.5f;
                    float playerRight = player.pos.x + player.width * 0.5f;
                    bool overlapX = (playerRight >= pfLeft) && (playerLeft <= pfRight);
                    bool landedThisFrame = (playerPrevBottom >= pfTop) && (playerCurrBottom <= pfTop);
                    if (overlapX && landedThisFrame)
                    {
                        player.pos.y = pfTop + player.height * 0.5f;
                        player.vel.y = 0.0f;
                        player.grounded = 1;
                        return true;
                    }
                }
                return false;
            };

        if (checkSet(platforms1)) return true;
        if (checkSet(platforms2)) return true;
        if (checkSet(platforms3)) return true;
        return checkSet(platforms4);
    }

    // ------------------------------------------------------------------------
    // Walls
    void HandleWalls(Player& player, const std::vector<Platform>& walls)
    {
        WallCollisionCheck(player, walls);   // now from Platforms.h
    }

    // ------------------------------------------------------------------------
    // Obstacles
    bool HandleObstacles(Player& player, const std::vector<PlatformObstacle>& obstacles)
    {
        return CheckObstacleCollision(player, obstacles);   // from Platforms.h
    }

    // ------------------------------------------------------------------------
    // Checkpoints
    bool HandleCheckpoints(Player& player, const std::vector<Checkpoint>& checkpoints)
    {
        return CheckpointCollisionCheck(player, checkpoints);   // from Platforms.h
    }

    // ------------------------------------------------------------------------
    // Buttons
    void HandleButtons(Player& player, std::vector<PlatformButton>& buttons, std::vector<Platform>& platforms)
    {
        for (auto& btn : buttons)
        {
            float btnLeft = btn.x - btn.w * 0.5f;
            float btnRight = btn.x + btn.w * 0.5f;
            float btnTop = btn.y + btn.h * 0.5f;
            float btnBottom = btn.y - btn.h * 0.5f;
            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerBottom = player.pos.y - player.height * 0.5f;

            bool overlapX = (playerRight >= btnLeft) && (playerLeft <= btnRight);
            bool landedOnButton = (playerBottom <= btnTop) && (playerBottom >= btnBottom);
            bool isPressed = overlapX && landedOnButton && player.grounded;

            if (isPressed && !btn.wasPressed)
            {
                platforms[btn.platformIndex].active = !platforms[btn.platformIndex].active;
            }
            btn.wasPressed = isPressed;
        }
    }

    // ------------------------------------------------------------------------
    // Player vs Enemy (hard/boss)
    void HandlePlayerEnemyCollisions(Player& player, std::vector<Enemy>& enemies)
    {
        for (auto& enemy : enemies)
        {
            if (!enemy.isAlive) continue;
            if (enemy.type == EnemyType::Hard || enemy.type == EnemyType::Boss)
            {
                float playerHalfW = player.width * 0.5f;
                float playerHalfH = player.height * 0.5f;
                float enemyHalfW = enemy.width * 0.5f;
                float enemyHalfH = enemy.height * 0.5f;
                bool overlapX = fabs(enemy.pos.x - player.pos.x) < (enemyHalfW + playerHalfW);
                bool overlapY = fabs(enemy.pos.y - player.pos.y) < (enemyHalfH + playerHalfH);
                if (overlapX && overlapY)
                {
                    HardEnemy_OnCollision(enemy, player);   // from HardEnemy.h
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Enemy bullets vs Player
    void HandleEnemyBulletPlayerCollisions(std::vector<EnemyBullet>& bullets, Player& player)
    {
        for (auto& bullet : bullets)
        {
            if (!bullet.active) continue;
            float bulletHalfW = 20.0f;  // should come from bullet size
            float bulletHalfH = 20.0f;
            float playerHalfW = player.width * 0.5f;
            float playerHalfH = player.height * 0.5f;
            bool overlapX = fabs(bullet.pos.x - player.pos.x) < (bulletHalfW + playerHalfW);
            bool overlapY = fabs(bullet.pos.y - player.pos.y) < (bulletHalfH + playerHalfH);
            if (overlapX && overlapY)
            {
                Player_ApplyDamage(player, bullet.damage);   // from Player.h
                bullet.active = false;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Player bullets vs Enemies
    void HandlePlayerBulletEnemyCollisions(Player& player, std::vector<Enemy>& enemies)
    {
        for (auto& enemy : enemies)
        {
            if (enemy.isAlive)
            {
                Player_CheckBulletCollisions(player, enemy);   // from PlayerBullet.h
            }
        }
    }

    // ------------------------------------------------------------------------
    // Player melee vs Enemies
    void HandlePlayerMeleeEnemyCollisions(Player& player, std::vector<Enemy>& enemies)
    {
        std::vector<Enemy*> enemyPtrs;
        for (auto& e : enemies) enemyPtrs.push_back(&e);
        PlayerMelee_CheckCollisions(player, enemyPtrs);   // from PlayerMelee.h
    }

    CollisionResults HandleAllCollisions(
        Player& player,
        float playerPrevY,
        EnvironmentManager& env,
        std::vector<Enemy>& enemies,
        std::vector<EnemyBullet>& enemyBullets)
    {
        CollisionResults results = { false, false };

        // Ground (fixed values – could become env constants)
        HandleGround(player, -350.0f, 50.0f, playerPrevY);

        // Platforms (standard falling check)
        HandlePlatforms(player, playerPrevY, env.GetLevel1Platforms());

        // Landing on any platform (combined correction)
        HandleLandingOnAnyPlatform(player, playerPrevY,
            env.GetLevel1Platforms(),
            env.GetLevel2Platforms(),
            env.GetLevel3Platforms(),
            env.GetBossPlatforms());

        // Obstacles – store result
        results.obstacleHit = HandleObstacles(player, env.GetObstacles());

        // Walls
        HandleWalls(player, env.GetWallPlatforms());

        // Checkpoints – store result
        results.checkpointHit = HandleCheckpoints(player, env.GetCheckpoints());

        // Buttons – these modify env's buttons and level2 platforms
        HandleButtons(player, env.GetLevel2Buttons(), env.GetLevel2Platforms());

        // Enemy collisions
        HandlePlayerEnemyCollisions(player, enemies);
        HandleEnemyBulletPlayerCollisions(enemyBullets, player);
        HandlePlayerBulletEnemyCollisions(player, enemies);
        HandlePlayerMeleeEnemyCollisions(player, enemies);

        return results;
    }
}
/* Start Header ************************************************************************/
/*!
\file       CollisionManager.cpp
\author     Joash ng, joash.ng, 2502780
            Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
            Sim Hui Min, s.huimin, 2503506
            Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par        joash.ng@digipen.edu
            p.yuxuanlovette@digipen.edu
            s.huimin@digipen.edu
            kerwinjiajie.wong@digipen.edu
\date       Feb 26 2026
\brief		This file contains implementations for collision detection and response
            for all game entities: player, enemies, bullets, platforms, and obstacles.
            It includes AABB checks, spatial grid optimization, and pogo mechanics.
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "CollisionManager.h"
#include "EnvironmentManager.h"
#include "SpatialGrid.h"
#include "ObjectManager.h"
#include "Player.h"             // for Player_ApplyDamage
#include "Camera.h"             // for Camera_AddTrauma
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
    // Landing on any platform from multiple sets (combined correction)
    bool HandleLandingOnAnyPlatform(Player& player, float playerPrevY,
        const std::vector<Platform>& platforms1,
        const std::vector<Platform>& platforms2,
        const std::vector<Platform>& platforms3,
        const std::vector<Platform>& platforms4)
    {

        auto checkSet = [&](const std::vector<Platform>& platforms) -> bool
            {
                for (const Platform& pf : platforms)
                {
                    if (!pf.active) continue;

                    float pfLeft = pf.x - pf.w * 0.5f;
                    float pfRight = pf.x + pf.w * 0.5f;
                    float pfTop = pf.y + pf.h * 0.5f;
                    float pfBottom = pf.y - pf.h * 0.5f;

                    float playerPrevTop = playerPrevY + player.height * 0.5f;
                    float playerCurrTop = player.pos.y + player.height * 0.5f;

                    float playerPrevBottom = playerPrevY - player.height * 0.5f;
                    float playerCurrBottom = player.pos.y - player.height * 0.5f;

                    // horizontal overlap
                    float collisionHalfW = player.width * 0.2f;
                    float playerLeft = player.pos.x - collisionHalfW;
                    float playerRight = player.pos.x + collisionHalfW;
                   
                    bool overlapX = (playerRight > pfLeft) && (playerLeft < pfRight);
                    if (!overlapX) continue;

                    // check if player is standing on platform
                     float tolerance = 0.1f;
                     if (fabs(playerCurrBottom - pfTop) <= tolerance && overlapX)
                    {
                        player.grounded = 1;
                        player.pos.y = pfTop + player.height * 0.5f;
                        return true;
                    }

                    // collision against top of platform
                    if (player.vel.y <= 0.0f && playerPrevBottom >= pfTop && playerCurrBottom < pfTop)
                    {
                        player.pos.y = pfTop + player.height * 0.5f;
                        player.vel.y = 0.0f;
                        player.grounded = 1;
                        return true;
                    }

                    // collision against bottom of platform
                    if (player.vel.y > 0.0f && playerPrevTop <= pfBottom && playerCurrTop > pfBottom)
                    {
                        player.pos.y = pfBottom - player.height * 0.5f;
                        player.vel.y = 0.0f;
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
    // Lasers
    void HandlePlayerLaserCollisions(Player& player, const std::vector<PlatformLaser>& lasers)
    {
        for (const auto& ls : lasers)
        {
            if (!ls.laserActive) continue;

            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;
            float playerBot = player.pos.y - player.height * 0.5f;

            float laserLeft = std::min(ls.x1, ls.x2) - ls.w * 0.5f;
            float laserRight = std::max(ls.x1, ls.x2) + ls.w * 0.5f;
            float laserBot = std::min(ls.y1, ls.y2);
            float laserTop = std::max(ls.y1, ls.y2);

            bool overlapX = (playerRight > laserLeft) && (playerLeft < laserRight);
            bool overlapY = (playerTop > laserBot) && (playerBot < laserTop);

            if (overlapX && overlapY)
            {
                float pushRight = laserRight - playerLeft;
                float pushLeft = playerRight - laserLeft;

                if (pushRight < pushLeft)
                {
                    player.pos.x += pushRight;
                }
                else
                {
                    player.pos.x -= pushLeft;
                }

                float midX = (ls.x1 + ls.x2) * 0.5f;
                float midY = (ls.y1 + ls.y2) * 0.5f;

                Player_ApplyDamage(player, 1.0f);
                Camera_AddTrauma(0.6f);
                Player_ApplyKnockback(player, midX, midY);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Buttons
    void HandleButtons(Player& player, const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms)
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
            float playerTop = player.pos.y + player.height * 0.5f;

            // check if player is near/overlapping switch
            bool overlapX = (playerRight >= btnLeft) && (playerLeft <= btnRight);
            bool overlapY = (playerTop >= btnBottom) && (playerBottom <= btnTop);
            bool inRange = overlapX && overlapY;

            if (inRange && AEInputCheckTriggered('E'))
            {
                for (int index : btn.platformIndices) {

                    if (index >= 0 && index < (int)platforms.size()) {

                        platforms[index].active = !platforms[index].active;
                        EnvironmentManager::Get().MarkStaticDirty();

                    }

                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Computers
    void HandleComputers(Player& player, const std::vector<PlatformComputer>& computers, const std::vector<PlatformLaser>& lasers)
    {
        for (auto& comp : computers)
        {
            float compLeft = comp.x - comp.w * 0.5f;
            float compRight = comp.x + comp.w * 0.5f;
            float compTop = comp.y + comp.h * 0.5f;
            float compBot = comp.y - comp.h * 0.5f;

            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;
            float playerBot = player.pos.y - player.height * 0.5f;

            bool overlapX = (playerRight >= compLeft) && (playerLeft <= compRight);
            bool overlapY = (playerTop >= compBot) && (playerBot <= compTop);

            if (overlapX && overlapY && AEInputCheckTriggered('E'))
            {
                for (int index : comp.laserIndices)
                {
                    if (index >= 0 && index < (int)lasers.size())
                    {
                        lasers[index].laserActive = !lasers[index].laserActive;
                    }
                }
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

    //-------------------------------------------------------------------------
    //---------------------SPATIAL GRID VERSIONS-------------------------------
    //-------------------------------------------------------------------------


    // ------------------------------------------------------------------------
    // Spatial grid versions - only check objects in nearby cells
    // ------------------------------------------------------------------------
    void HandlePlatformsSpatial(Player& player, float playerPrevY, const SpatialGrid& grid)
    {
        std::vector<const Platform*> nearby;
        grid.GetNearbyPlatforms(player.pos.y, player.height, nearby);

        for (const Platform* pf : nearby)
        {
            if (!pf->active) continue;


            float pfLeft = pf->x - pf->w * 0.5f;
            float pfRight = pf->x + pf->w * 0.5f;
            float pfTop = pf->y + pf->h * 0.5f;
            float pfBottom = pf->y - pf->h * 0.5f;

            float playerPrevTop = playerPrevY + player.height * 0.5f;
            float playerCurrTop = player.pos.y + player.height * 0.5f;

            float playerPrevBottom = playerPrevY - player.height * 0.5f;
            float playerCurrBottom = player.pos.y - player.height * 0.5f;

            // horizontal overlap
            float collisionHalfW = player.width * 0.2f;
            float playerLeft = player.pos.x - collisionHalfW;
            float playerRight = player.pos.x + collisionHalfW;

            bool overlapX = (playerRight > pfLeft) && (playerLeft < pfRight);
            if (!overlapX) continue;

            // check if player is standing on platform
            float tolerance = 0.1f;
            if (fabs(playerCurrBottom - pfTop) <= tolerance && overlapX)
            {
                player.grounded = 1;
                player.pos.y = pfTop + player.height * 0.5f;
                return;
            }

            // collision against top of platform
            if (player.vel.y <= 0.0f && playerPrevBottom >= pfTop && playerCurrBottom < pfTop)
            {
                player.pos.y = pfTop + player.height * 0.5f;
                player.vel.y = 0.0f;
                player.grounded = 1;
                return;
            }

            // collision against bottom of platform
            if (player.vel.y > 0.0f && playerPrevTop <= pfBottom && playerCurrTop > pfBottom)
            {
                player.pos.y = pfBottom - player.height * 0.5f;
                player.vel.y = 0.0f;
                return;
            }
        }
    }


    bool HandleObstaclesSpatial(Player& player, const SpatialGrid& grid)
    {
        std::vector<const PlatformObstacle*> nearby;
        grid.GetNearbyObstacles(player.pos.y, player.height, nearby);

        for (const PlatformObstacle* obs : nearby)
        {
            float obsLeft = obs->x - obs->w * 0.5f;
            float obsRight = obs->x + obs->w * 0.5f;
            float obsTop = obs->y + obs->h * 0.5f;
            float obsBottom = obs->y - obs->h * 0.5f;
            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;
            float playerBottom = player.pos.y - player.height * 0.5f;

            bool overlapX = (playerRight >= obsLeft) && (playerLeft <= obsRight);
            bool overlapY = (playerTop >= obsBottom) && (playerBottom <= obsTop);

            if (overlapX && overlapY)
            {
                return true;
            }
        }
        return false;
    }

    void HandleCheckpointsSpatial(Player& player, const SpatialGrid& grid, bool& checkpointHit, bool& checkpointInRange)
    {
        checkpointHit = false;
        checkpointInRange = false;
        const Checkpoint* nearestInRange = nullptr;

        std::vector<const Checkpoint*> nearby;
        grid.GetNearbyCheckpoints(player.pos.y, player.height, nearby);

        for (const Checkpoint* cp : nearby)
        {
            float cpLeft = cp->x - cp->w * 0.5f;
            float cpRight = cp->x + cp->w * 0.5f;
            float cpTop = cp->y + cp->h * 0.5f;
            float cpBottom = cp->y - cp->h * 0.5f;
            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;
            float playerBottom = player.pos.y - player.height * 0.5f;

            bool overlapX = (playerRight >= cpLeft) && (playerLeft <= cpRight);
            bool overlapY = (playerTop >= cpBottom) && (playerBottom <= cpTop);

            if (overlapX && overlapY)
            {
                checkpointHit = true;
            }

            float rangeLeft = cp->x - cp->w;
            float rangeRight = cp->x + cp->w;
            float rangeTop = cp->y + cp->h;
            float rangeBottom = cp->y - cp->h;

            bool inRangeX = (playerRight >= rangeLeft) && (playerLeft <= rangeRight);
            bool inRangeY = (playerTop >= rangeBottom) && (playerBottom <= rangeTop);

            if (inRangeX && inRangeY && !nearestInRange)
            {
                checkpointInRange = true;
                nearestInRange = cp;
            }
        }
    }

    void HandlePlayerEnemyCollisionsSpatial(Player& player, const SpatialGrid& grid)
    {
        std::vector<Enemy*> nearby;
        grid.GetNearbyEnemies(player.pos.y, player.height, nearby);

        for (auto* enemy : nearby)
        {
            if (!enemy->isAlive) continue;
            if (enemy->type == EnemyType::Hard || enemy->type == EnemyType::Boss)
            {
                float playerHalfW = player.width * 0.5f;
                float playerHalfH = player.height * 0.5f;
                float enemyHalfW = enemy->width * 0.5f;
                float enemyHalfH = enemy->height * 0.5f;
                bool overlapX = fabs(enemy->pos.x - player.pos.x) < (enemyHalfW + playerHalfW);
                bool overlapY = fabs(enemy->pos.y - player.pos.y) < (enemyHalfH + playerHalfH);
                if (overlapX && overlapY)
                {
                    HardEnemy_OnCollision(*enemy, player);
                }
            }
        }
    }

    void HandleEnemyBulletPlayerCollisionsSpatial(Player& player, const SpatialGrid& grid)
    {
        std::vector<EnemyBullet*> nearbyBullets;
        grid.GetNearbyBullets(player.pos.y, player.height, nearbyBullets);

        for (auto* bullet : nearbyBullets)
        {
            if (!bullet->active) continue;
            float bulletHalfW = bullet->width * 0.5f;
            float bulletHalfH = bullet->height * 0.5f;
            float playerHalfW = player.width * 0.5f;
            float playerHalfH = player.height * 0.5f;
            bool overlapX = fabs(bullet->pos.x - player.pos.x) < (bulletHalfW + playerHalfW);
            bool overlapY = fabs(bullet->pos.y - player.pos.y) < (bulletHalfH + playerHalfH);
            if (overlapX && overlapY)
            {
                Player_ApplyDamage(player, bullet->damage);
                Camera_AddTrauma(0.6f);
                Player_ApplyKnockback(player, bullet->pos.x, bullet->pos.y);
                bullet->active = false;
            }
        }
    }

    bool CollisionManager::HandlePogoCollisionSpatial(Player& player, const SpatialGrid& grid)
    {
        // Only pogo if down-slashing in mid-air
        if (!player.isAttacking || player.grounded || player.slashDirection != SlashDirection::DOWN)
            return false;

        // get player's downslash hitbox (slightly below player)
        float slashX = player.pos.x;
        float slashY = player.pos.y - player.height * 0.5f - 20.0f;
        float slashW = player.width * 0.8f;
        float slashH = 40.0f;

        // Query nearby obstacles using the hitbox's centre and height
        std::vector<const PlatformObstacle*> nearby;
        grid.GetNearbyObstacles(slashY, slashH, nearby);

        // check collision with each spike
        for (const PlatformObstacle* obs : nearby)
        {
            if (!obs->isSpike) continue;   // skip non-spike obstacles

            // AABB collision
            float distX = fabs(slashX - obs->x);
            float distY = fabs(slashY - obs->y);
            float needX = slashW * 0.5f + obs->w * 0.5f;
            float needY = slashH * 0.5f + obs->h * 0.5f;

            if (distX < needX && distY < needY)
                return true; //hit spike
        }
        return false;
    }

    CollisionResults HandleAllCollisionsSpatial(
        Player& player,
        float playerPrevY,
        EnvironmentManager& env,
        std::vector<Enemy>& enemies)
    {
        CollisionResults results = { false, false, false };

        player.grounded = 0;

        HandleGround(player, -350.0f, 50.0f, playerPrevY);

        const SpatialGrid& grid = env.GetSpatialGrid();
        HandlePlatformsSpatial(player, playerPrevY, grid);

        HandleLandingOnAnyPlatform(player, playerPrevY,
            env.GetLevel1Platforms(),
            env.GetLevel2Platforms(),
            env.GetLevel3Platforms(),
            env.GetBossPlatforms());

        results.obstacleHit = HandleObstaclesSpatial(player, grid);
        HandleCheckpointsSpatial(player, grid, results.checkpointHit, results.checkpointInRange);
        results.pogoHit = HandlePogoCollisionSpatial(player, grid);

        HandleWalls(player, env.GetWallPlatforms()); //small number, can just use non-spatial version
        HandleWalls(player, env.GetLevel3WallPlatforms());

        // Buttons - these modify env's buttons and platforms
        HandleButtons(player, env.GetLevel1Buttons(), env.GetLevel1Platforms());
        HandleButtons(player, env.GetLevel2Buttons(), env.GetLevel2Platforms()); //small no. too
        HandleButtons(player, env.GetLevel3Buttons(), env.GetLevel3Platforms());

        HandlePlayerEnemyCollisionsSpatial(player, grid);
        HandleEnemyBulletPlayerCollisionsSpatial(player, grid);
        HandlePlayerBulletEnemyCollisions(player, enemies); // small no. of bullets
        HandlePlayerMeleeEnemyCollisions(player, enemies); //small no. of bullets

        HandlePlayerLaserCollisions(player, env.GetLevel2Lasers());
        HandlePlayerLaserCollisions(player, env.GetLevel3Lasers());

        HandleComputers(player, env.GetLevel3Computers(), env.GetLevel3Lasers());

        return results;
    }
}
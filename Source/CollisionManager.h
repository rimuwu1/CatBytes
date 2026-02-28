/* Start Header ************************************************************************/
/*!
\file       CollisionManager.h
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date       Feb 26 2026
\brief		This file declares all the collision checks under the collsion namespace.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "Player.h"
#include "enemy.h"
#include "Platforms.h"
#include "EnemyBullet.h"
#include "PlayerBullet.h"
#include "PlayerMelee.h"
#include "EnvironmentManager.h"
#include <vector>

namespace CollisionManager
{
    // Ground collision
    void HandleGround(Player& player, float groundY, float groundHeight, float& playerPrevY);

    // Platform collisions (landing and standing)
    void HandlePlatforms(Player& player, float playerPrevY, const std::vector<Platform>& platforms);
    // Check landing on any of multiple platform sets (used for combined check)
    bool HandleLandingOnAnyPlatform(Player& player, float playerPrevY,
                                    const std::vector<Platform>& platforms1,
                                    const std::vector<Platform>& platforms2,
                                    const std::vector<Platform>& platforms3,
                                    const std::vector<Platform>& platforms4);

    // Wall collisions (horizontal push)
    void HandleWalls(Player& player, const std::vector<Platform>& walls);

    // Obstacle collisions (deadly)
    bool HandleObstacles(Player& player, const std::vector<PlatformObstacle>& obstacles);

    // Checkpoint collisions
    bool HandleCheckpoints(Player& player, const std::vector<Checkpoint>& checkpoints);

    // Button collisions (toggle platforms)
    void HandleButtons(Player& player, std::vector<PlatformButton>& buttons, std::vector<Platform>& platforms);

    // Enemy (hard/boss) collisions with player
    void HandlePlayerEnemyCollisions(Player& player, std::vector<Enemy>& enemies);

    // Enemy bullet collisions with player
    void HandleEnemyBulletPlayerCollisions(std::vector<EnemyBullet>& bullets, Player& player);

    // Player bullet collisions with enemies
    void HandlePlayerBulletEnemyCollisions(Player& player, std::vector<Enemy>& enemies);

    // Player melee collisions with enemies
    void HandlePlayerMeleeEnemyCollisions(Player& player, std::vector<Enemy>& enemies);

    //----------wrapper for all collisions------------
    struct CollisionResults {
        bool obstacleHit;   // true if player hit an obstacle
        bool checkpointHit; // true if player touched a checkpoint
    };

    CollisionResults HandleAllCollisions(
        Player& player,
        float playerPrevY,
        EnvironmentManager& env,
        std::vector<Enemy>& enemies,
        std::vector<EnemyBullet>& enemyBullets
    );

}
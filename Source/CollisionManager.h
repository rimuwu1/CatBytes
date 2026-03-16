/* Start Header ************************************************************************/
/*!
\file       CollisionManager.h
\author     Joash ng, joash.ng, 2502780
            Sim Hui Min, s.huimin, 2503506
\par        joash.ng@digipen.edu
            s.huimin@digipen.edu
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
#include "SpatialGrid.h"
#include <vector>

namespace CollisionManager
{
    // Ground collision
    void HandleGround(Player& player, float groundY, float groundHeight, float& playerPrevY);



    bool HandleLandingOnAnyPlatform(Player& player, float playerPrevY,
                                    const std::vector<Platform>& platforms1,
                                    const std::vector<Platform>& platforms2,
                                    const std::vector<Platform>& platforms3,
                                    const std::vector<Platform>& platforms4);

    // Wall collisions (horizontal push)
    void HandleWalls(Player& player, const std::vector<Platform>& walls);

    // Laser collisions
    void HandlePlayerLaserCollisions(Player& player, const std::vector<PlatformLaser>& lasers);

    // Button collisions (toggle platforms)
    void HandleButtons(Player& player, const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms);

    // Player bullet collisions with enemies
    //void HandlePlayerBulletEnemyCollisions(Player& player, std::vector<Enemy>& enemies);

    // Player melee collisions with enemies
    void HandlePlayerMeleeEnemyCollisions(Player& player, std::vector<Enemy>& enemies);

    //---------- Spatial grid versions (use for optimization) ------------
    void HandlePlatformsSpatial(Player& player, float playerPrevY, const SpatialGrid& grid);
    bool HandleObstaclesSpatial(Player& player, const SpatialGrid& grid);
    void HandleCheckpointsSpatial(Player& player, const SpatialGrid& grid, bool& checkpointHit, bool& checkpointInRange);
    void HandlePlayerEnemyCollisionsSpatial(Player& player, const SpatialGrid& grid);
    void HandleEnemyBulletPlayerCollisionsSpatial(Player& player, const SpatialGrid& grid);
    bool HandlePogoCollisionSpatial(Player& player, const SpatialGrid& grid);

    //----------wrapper for all collisions------------
    struct CollisionResults {
        bool obstacleHit;           // true if player hit an obstacle
        bool checkpointHit;         // true if player touched (overlapped) a checkpoint
        bool checkpointInRange;     // true if player is near a checkpoint (2x range)
        bool pogoHit;               // true if pogo performed
    };


    CollisionResults HandleAllCollisionsSpatial(
        Player& player,
        float playerPrevY,
        EnvironmentManager& env,
        std::vector<Enemy>& enemies
    );

}
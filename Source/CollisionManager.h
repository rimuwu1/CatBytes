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
#include "Enemy.h"
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

    // Boss laser collisions (boss is an enemy instance)
    void HandleBossLaserCollisions(Player& player, Enemy& boss);

    // Toggle type for button/computer interactions
    enum class ToggleType {
        None,
        Platform,       // button toggles platforms
        Wall,           // button toggles toggle walls
        Laser,          // computer toggles lasers
        BossDoorUnlock  // boss-door pc was activated and unlocked the boss door
    };

    // Button toggle result - returns position and type of toggled element
    struct ButtonToggleResult {
        bool triggered = false;
        float buttonX  = 0.0f;  // button/computer X position for matching
        float buttonY  = 0.0f;  // button/computer Y position for matching
        float targetY  = 0.0f;  // camera pan target Y
        ToggleType type = ToggleType::None;
    };

    // Button collisions (toggle platforms or walls)
    ButtonToggleResult HandleButtons(Player& player, const std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms, const std::vector<Platform>& toggleWalls);

    // Computer collisions (toggle lasers) - returns result for camera pan
    ButtonToggleResult HandleComputers(Player& player, const std::vector<PlatformComputer>& computers);

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
        bool obstacleHit      = false;
        bool pogoHit          = false;
        bool checkpointInRange = false;
        bool checkpointHit    = false;
        ButtonToggleResult pendingToggle;   // for buttons (platforms/walls)
        ButtonToggleResult pendingComputer; // for computers (lasers)
        bool pendingCameraPan = false;
        float cameraPanTargetY = 0.0f;
    };


    CollisionResults HandleAllCollisionsSpatial(
        Player& player,
        float playerPrevY,
        EnvironmentManager& env,
        std::vector<Enemy>& enemies
    );

}
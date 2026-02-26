/* Start Header ************************************************************************/
/*!
\file PlayerMelee.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief Declares functions and structures for handling the player's melee attacks,
       including hit detection, attack state, and damage application.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Player.h"
#include "enemy.h"
#include "Boss.h"
#include <vector>

// ----------------------------------------------------------------------------
// Initializes the player's melee attack state
// Resets attack flags and timers.
// ----------------------------------------------------------------------------
void PlayerMelee_Init(Player& player);

// ----------------------------------------------------------------------------
// Handles melee attack logic for a single enemy.
// Only processes if the player is attacking, has a melee weapon,
// and the enemy is alive. Applies damage and sets hit flag
// ----------------------------------------------------------------------------
void PlayerMelee_Update(Player& player, Enemy& enemy);

// ----------------------------------------------------------------------------
// Handles melee attacks against multiple enemies
// Calls PlayerMelee_Update for each enemy in the list
// ----------------------------------------------------------------------------
void PlayerMelee_CheckCollisions(Player& player, std::vector<Enemy*>& enemies);




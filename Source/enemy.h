/* Start Header ************************************************************************/
/*!
\file enemy.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
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
//#include "Player.h"

//forward declare Player
struct Player;

// ----------------------------------------------------------------------------
// Enemy struct
// Represents a simple patrolling enemy
// ----------------------------------------------------------------------------
struct Enemy
{
    bool facesLeft = false; // for flipping images whille they're patrolling

    AEGfxVertexList* mesh = nullptr;

    //per-enemy textures
    AEGfxTexture* texture = nullptr;//current texture
    AEGfxTexture* normalTexture = nullptr;//default texture
    AEGfxTexture* attackTexture = nullptr;//for HardEnemy attack

    AEVec2 pos = { 0.0f, 0.0f };//enemy position in world coordinates
    float width = 0.0f; //enemy width
    float height = 0.0f;

    float moveSpeed = 0.0f;//Movement speed in units per second
    int direction = 1;//current movement direction: -1 = left, 1 = right
    int isAlive = 0;//status: 1=alive, 0=dead

    float hitPoints = 0.0f; // number of hits left
    float hitStunTimer = 0.0f; // time remaining frozen after hit
    bool isPlayerColliding = false; // prevents multiple hits per frame

    float shootCooldown = 0.0f;//time between shots
    float shootTimer = 0.0f; //countdown until next shot

    // HardEnemy only
    float damage = 0.0f; //damage to deal to player on collision
};

// ----------------------------------------------------------------------------
// initializes an enemy at a given position
// loads movement speed from file, sets alive state, creates mesh/texture if needed
// ----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, float startX, float startY);

// ----------------------------------------------------------------------------
// Updates enemy every frame
// moves left/right automatically within patrol bounds
// handles hit stun freeze
// ----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt);

// ----------------------------------------------------------------------------
// draws the enemy on screen
// shows low HP overlay when hit or at 1 HP
// ----------------------------------------------------------------------------
void Enemy_Draw(const Enemy& enemy);

// ----------------------------------------------------------------------------
// called when player collides with enemy
// reduces hitPoints and sets hit stun
// ----------------------------------------------------------------------------
void Enemy_OnHit(Enemy& enemy, float damage);

// ----------------------------------------------------------------------------
// frees static resources (mesh and texture) shared across all enemies
// ----------------------------------------------------------------------------
void Enemy_Free();

// -----------------------------------------------------------------------------
// Initialize HardEnemy
// Similar to EasyEnemy but uses HardEnemy files, higher speed, HP, and attack texture
// -----------------------------------------------------------------------------
void HardEnemy_Init(Enemy& enemy, float startX, float startY);

// -----------------------------------------------------------------------------
// Update HardEnemy
// Patrols like EasyEnemy, stops for 0.3s on collision, then continues
// -----------------------------------------------------------------------------
void HardEnemy_Update(Enemy& enemy, float dt);

// -----------------------------------------------------------------------------
// HardEnemy collision with player
// Damage player and pause movement for 0.3s with attack texture
// -----------------------------------------------------------------------------
void HardEnemy_OnCollision(Enemy& enemy, Player& player);
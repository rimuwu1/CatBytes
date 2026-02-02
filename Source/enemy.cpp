/* Start Header ************************************************************************/
/*!
\file enemy.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief Implements a simple patrolling(?) enemy.
The enemy moves automatically left and right between patrol bounds 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "enemy.h"
#include "Utils.h"
#include "Player.h"
#include <fstream>
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"

extern rapidjson::Document level1Config;

// -----------------------------------------------------------------------------
// initialize enemy
// sets position, size, direction, alive status, loads speed and textures
// -----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, float startX, float startY)
{
    enemy.facesLeft = true; // for flipping img. easy enemy (veryslightly if u squint..) faces left

    enemy.pos = { startX, startY };// set position
    enemy.width = 80.0f;//enemy width
    enemy.height = 80.0f;//enemy height

    //load movement speed from file; default to 100 if file missing or invalid
    enemy.moveSpeed = level1Config["level_1"]["enemies"][0]["speed"].GetFloat();
    if (enemy.moveSpeed <= 0.0f) enemy.moveSpeed = 100.0f;

    enemy.shootCooldown = 1.5f;//shoots every 1.5 seconds
    enemy.shootTimer = enemy.shootCooldown;

    //load hit points from file; default to 3.0
    enemy.hitPoints = level1Config["level_1"]["enemies"][0]["hp"].GetFloat();
    if (enemy.hitPoints <= 0.0f) enemy.hitPoints = 3.0f;

    enemy.direction = 1;//start moving right
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;
}

// -----------------------------------------------------------------------------
// Update enemy: automatic left/right patrol
// -----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt)
{
    enemy.facesLeft = false; // hard enemy jpg faces right (for now anyways)

    if (!enemy.isAlive) return;//skip if dead

    //handle hit stun (freeze)
    if (enemy.hitStunTimer > 0.0f)
    {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer <= 0.0f) enemy.hitStunTimer = 0.0f;
        return; // do not move while stunned
    }

    enemy.shootTimer -= dt;

    //time to shoot
    if (enemy.shootTimer <= 0.0f)
    {
        //tell the level to spawn a bullet(not here)
        enemy.shootTimer = enemy.shootCooldown;
    }

    //move horizontally based on direction and speed
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;

    //patrol bounds (hardcoded)
    float patrolMinX = -400.0f;
    float patrolMaxX = 400.0f;

    //flip direction when reaching patrol bounds
    if (enemy.pos.x >= patrolMaxX)
        enemy.direction = -1;//move left
    else if (enemy.pos.x <= patrolMinX)
        enemy.direction = 1;// move right
}

// -----------------------------------------------------------------------------
// Draw enemy on screen
// -----------------------------------------------------------------------------
void Enemy_Draw(const Enemy& enemy)
{
    if (!enemy.isAlive) return;//Skip if dead

    // flip when moving opposite to the way the image faces
    float scaleX;
    if (enemy.facesLeft)
        scaleX = (enemy.direction == 1) ? -enemy.width : enemy.width;
    else
        scaleX = (enemy.direction == -1) ? -enemy.width : enemy.width;

    util::DrawTexturedSquare(
        enemy.mesh,
        enemy.texture, // overlay handled by level, not enemy
        enemy.pos.x,
        enemy.pos.y,
        enemy.width,
        enemy.height
    );

    {
        float alpha = (enemy.hitPoints == 1) ? 0.2f : (enemy.hitStunTimer / 0.5f) * 0.2f;
        util::DrawTexturedSquare(
            enemy.mesh,
            enemy.texture,
            enemy.pos.x,
            enemy.pos.y,
            enemy.width,
            enemy.height,
            alpha
        );
    }
}

// -----------------------------------------------------------------------------
// called when player collides with enemy
// reduces hitPoints and sets hit stun
// -----------------------------------------------------------------------------
void Enemy_OnHit(Enemy& enemy, float damage)
{
    if (!enemy.isAlive || enemy.hitStunTimer > 0.0f)
        return; // already hit, or dead

    // decrease hit points
    enemy.hitPoints -=damage;

    // freeze enemy for 0.5 seconds
    enemy.hitStunTimer = 0.5f;

    // if HP <= 0, enemy dies
    if (enemy.hitPoints <= 0)
        enemy.isAlive = 0;
}



void HardEnemy_Init(Enemy& enemy, float startX, float startY)
{
    enemy.pos = { startX, startY };
    enemy.width = 80.0f;
    enemy.height = 80.0f;

    //load speed and HP from files; fallback if missing
    enemy.moveSpeed = level1Config["level_1"]["enemies"][1]["speed"].GetFloat();
    if (enemy.moveSpeed <= 0.0f) enemy.moveSpeed = 100.0f;

    enemy.hitPoints = level1Config["level_1"]["enemies"][1]["hp"].GetFloat();
    if (enemy.hitPoints <= 0.0f) enemy.hitPoints = 5.0f;

    enemy.shootCooldown = 0.0f;//HardEnemy does collision damage instead
    enemy.shootTimer = 0.0f;
    enemy.direction = 1;
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;
    enemy.damage = level1Config["level_1"]["enemies"][1]["damage"].GetFloat();
    if (enemy.damage <= 0.0f) enemy.damage = 3.0f;//default damage for ahrd enemy
}


void HardEnemy_Update(Enemy& enemy, float dt)
{
    if (!enemy.isAlive) return;

    //handle hit stun/attack pause
    if (enemy.hitStunTimer > 0.0f)
    {
        enemy.hitStunTimer -= dt;

        //wheb stun finishes
        if (enemy.hitStunTimer <= 0.0f)
        {
            enemy.hitStunTimer = 0.0f;

            //switch back to normal texture
            enemy.texture = enemy.normalTexture;
        }

        return; // do not move while stunned
    }

    // patrol movement (only when NOT stunned)
    enemy.pos.x += enemy.direction * enemy.moveSpeed * dt;

    //patrol bounds
    float patrolMinX = -400.0f;
    float patrolMaxX = 400.0f;

    if (enemy.pos.x >= patrolMaxX)
        enemy.direction = -1;
    else if (enemy.pos.x <= patrolMinX)
        enemy.direction = 1;
}


void HardEnemy_OnCollision(Enemy& enemy, Player& player)
{
    // prevent repeated hits every frame
    if (!enemy.isAlive||enemy.hitStunTimer > 0.0f)
        return;

    // apply damage
    Player_ApplyDamage(player, enemy.damage);

    // switch to attack state
    enemy.hitStunTimer = 0.3f;
    enemy.texture = enemy.attackTexture;
}


// -----------------------------------------------------------------------------
// Free static resources (mesh and texture)
// -----------------------------------------------------------------------------
void Enemy_Free()
{
}
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
#include <fstream>
#include <iostream>

static AEGfxVertexList* enemyMesh = nullptr;
static AEGfxTexture* enemyTexture = nullptr;
static AEGfxTexture* lowHpOverlayTexture = nullptr;

// ----------------------------------------------------------------------------
// load a float value from a text file
// ----------------------------------------------------------------------------
static float LoadFloatFromFile(const char* filename)
{
    std::ifstream file(filename);
    float value = 0.0f;
    file >> value;
    return value;
}

// ----------------------------------------------------------------------------
// load an int value from a text file
// ----------------------------------------------------------------------------
static int LoadIntFromFile(const char* filename)
{
    std::ifstream file(filename);
    int value = 1;
    file >> value;
    return value;
}

// -----------------------------------------------------------------------------
// initialize enemy
// sets position, size, direction, alive status, loads speed and textures
// -----------------------------------------------------------------------------
void Enemy_Init(Enemy& enemy, float startX, float startY)
{
    enemy.pos = { startX, startY };// set position
    enemy.width = 50.0f;//enemy width
    enemy.height = 50.0f;//enemy height

    //load movement speed from file; default to 100 if file missing or invalid
    enemy.moveSpeed = LoadFloatFromFile("Assets/Data/EasyEnemySpeed.txt");
    if (enemy.moveSpeed <= 0.0f) enemy.moveSpeed = 100.0f;

    //load hit points from file; default to 3
    enemy.hitPoints = LoadIntFromFile("Assets/Data/EasyEnemyHitCount.txt");
    if (enemy.hitPoints <= 0) enemy.hitPoints = 3;

    enemy.direction = 1;//start moving right
    enemy.isAlive = 1;
    enemy.hitStunTimer = 0.0f;
    enemy.isPlayerColliding = false;

    //create mesh if it doesn't exist yet
    if (!enemyMesh)
        enemyMesh = util::CreateSquareMesh();

    //load enemy texture if not already loaded
    if (!enemyTexture)
        enemyTexture = AEGfxTextureLoad("Assets/Images/easyenemy.jpg");

    if (!lowHpOverlayTexture)
        lowHpOverlayTexture = AEGfxTextureLoad("Assets/Images/LowHpOverlay.jpg");
}

// -----------------------------------------------------------------------------
// Update enemy: automatic left/right patrol
// -----------------------------------------------------------------------------
void Enemy_Update(Enemy& enemy, float dt)
{
    if (!enemy.isAlive) return;//skip if dead

    //handle hit stun (freeze)
    if (enemy.hitStunTimer > 0.0f)
    {
        enemy.hitStunTimer -= dt;
        if (enemy.hitStunTimer < 0.0f) enemy.hitStunTimer = 0.0f;
        return; // do not move while stunned
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

    util::DrawTexturedSquare(
        enemyMesh,
        enemyTexture,
        enemy.pos.x,
        enemy.pos.y,
        enemy.width,
        enemy.height,
        1.0f //fully opaque
    );

    // Low HP overlay
    if (enemy.hitPoints == 1 || enemy.hitStunTimer > 0.0f)
    {
        float alpha = (enemy.hitPoints == 1) ? 0.2f : (enemy.hitStunTimer / 0.5f) * 0.2f;
        util::DrawTexturedSquare(
            enemyMesh,
            lowHpOverlayTexture,
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
void Enemy_OnHit(Enemy& enemy)
{
    if (!enemy.isAlive || enemy.hitStunTimer > 0.0f)
        return; // already hit, or dead

    // decrease hit points
    enemy.hitPoints--;

    // freeze enemy for 0.5 seconds
    enemy.hitStunTimer = 0.5f;

    // if HP <= 0, enemy dies
    if (enemy.hitPoints <= 0)
        enemy.isAlive = 0;
}

// -----------------------------------------------------------------------------
// Free static resources (mesh and texture)
// -----------------------------------------------------------------------------
void Enemy_Free()
{
    if (enemyTexture)
    {
        AEGfxTextureUnload(enemyTexture);
        enemyTexture = nullptr;
    }

    if (lowHpOverlayTexture)
    {
        AEGfxTextureUnload(lowHpOverlayTexture);
        lowHpOverlayTexture = nullptr;
    }

    if (enemyMesh)
    {
        AEGfxMeshFree(enemyMesh);
        enemyMesh = nullptr;
    }
}

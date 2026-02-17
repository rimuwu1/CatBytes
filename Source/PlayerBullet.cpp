/* Start Header *****/
/*!
\file       PlayerBullet.cpp
\author     Sim Hui Min, Huimin, s.huimin, 2503506
\par        s.huimin@digipen.edu
\date       February 01 2026
\brief      Implement the functions for the player's bullets

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#include "PlayerBullet.h"
#include "Utils.h"
#include "Player.h"

// shared resources
static AEGfxVertexList* bulletMesh = nullptr;
static AEGfxTexture* bulletTexture = nullptr;

void PlayerBullet_Init(PlayerBullet& bullet, const Player& player)
{
    bullet.active = false;
    bullet.width = player.bulletWidth;   
    bullet.height = player.bulletHeight;
    bullet.damage = player.bulletDamage; 
    bullet.pos = { 0.0f, 0.0f };
    bullet.vel = { 0.0f, 0.0f };
}


void PlayerBullet_Update(PlayerBullet& bullet, float dt)
{
    if (!bullet.active) return;

    bullet.pos.x += bullet.vel.x * dt;
    bullet.pos.y += bullet.vel.y * dt;

    // deactivate if off-screen
    if (bullet.pos.x > 850.0f || bullet.pos.x < -850.0f)
        bullet.active = false;
}

void PlayerBullet_Draw(const PlayerBullet& bullet)
{
    if (!bullet.active) return;

    if (!bulletMesh)
        bulletMesh = util::CreateSquareMesh();

    if (!bulletTexture)
        bulletTexture = AEGfxTextureLoad("Assets/Images/PlayerBullet.jpg");

    util::DrawTexturedSquare(
        bulletMesh,
        bulletTexture,
        bullet.pos.x,
        bullet.pos.y,
        bullet.width,
        bullet.height,
        1.0f
    );
}

void PlayerBullet_Free(PlayerBullet&)
{
    /*
    if (bulletTexture)
    {
        AEGfxTextureUnload(bulletTexture);
        bulletTexture = nullptr;
    }

    if (bulletMesh)
    {
        AEGfxMeshFree(bulletMesh);
        bulletMesh = nullptr;
    }
    */
}

//frees the static resources
void PlayerBullet_FreeShared()
{
    if (bulletTexture)
    {
        AEGfxTextureUnload(bulletTexture);
        bulletTexture = nullptr;
    }

    if (bulletMesh)
    {
        AEGfxMeshFree(bulletMesh);
        bulletMesh = nullptr;
    }
}

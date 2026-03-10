/* Start Header *****/
/*!
\file       PlayerBullet.cpp
\author     Sim Hui Min, Huimin, s.huimin, 2503506
            Joash ng, joash.ng, 2502780
\par        s.huimin@digipen.edu
            joash.ng@digipen.edu
\date       February 01 2026
\brief      Implement the functions for the player's bullets

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#include "PlayerBullet.h"
#include "SpriteSheet.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include "Player.h"

const float MAX_WINDOW_WIDTH = 850.0f;
const float MIN_WINDOW_WIDTH = -850.0f;

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
    if (bullet.pos.x > MAX_WINDOW_WIDTH || bullet.pos.x < MIN_WINDOW_WIDTH)
        bullet.active = false;
    if (bullet.bulletSprite)
        bullet.bulletSprite->Update(dt);
}

void PlayerBullet_Draw(const PlayerBullet& bullet)
{
    if (!bullet.active) return;

    // use sprite sheet if available, otherwise fallback to texture
    if (bullet.bulletSprite) {
        MeshManager::Get().DrawSpriteSheet(
            *bullet.bulletSprite,
            bullet.pos.x,
            bullet.pos.y,
            bullet.width,
            bullet.height,
            1.0f
        );
    }
    else {
        // fallback to static texture
        static AEGfxTexture* bulletTexture = TextureManager::Get().LoadTexture("Assets/Images/PlayerBullet.jpg");
        MeshManager::Get().DrawTexturedSquare(
            bulletTexture,
            bullet.pos.x,
            bullet.pos.y,
            bullet.width,
            bullet.height,
            1.0f
        );
    }
}

void PlayerBullet_Free(PlayerBullet&)
{
}

//frees the static resources
void PlayerBullet_FreeShared()
{
}

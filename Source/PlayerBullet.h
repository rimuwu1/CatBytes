/* Start Header *****/
/*!
\file       PlayerBullet.h
\author     Sim Hui Min, Huimin, s.huimin, 2503506
            Joash ng, joash.ng, 2502780
\par        s.huimin@digipen.edu
            joash.ng@digipen.edu
\date       February 01 2026
\brief      Declare functions for player bullets

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#pragma once
#include <memory>
#include "pch.h"

class SpriteSheet;
struct Player;

struct PlayerBullet {
    bool active = false;
    float width = 0.0f;
    float height = 0.0f;
    float damage = 0.0f;
    AEVec2 pos{ 0.0f, 0.0f };
    AEVec2 vel{ 0.0f, 0.0f };
    std::unique_ptr<SpriteSheet> bulletSprite;
};

void PlayerBullet_Init(PlayerBullet& bullet, const Player& player); 
void PlayerBullet_Update(PlayerBullet& bullet, float dt);
void PlayerBullet_Draw(const PlayerBullet& bullet);
void PlayerBullet_Free(PlayerBullet& bullet);
void PlayerBullet_FreeShared();
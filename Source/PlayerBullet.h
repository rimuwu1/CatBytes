/* Start Header *****/
/*!
\file       PlayerBullet.h
\author     Sim Hui Min, Huimin, s.huimin, 2503506
\par        s.huimin@digipen.edu
\date       February 01 2026
\brief      Declare functions for player bullets

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#pragma once
#include "AEEngine.h"
#include "Player.h"

struct PlayerBullet {
    bool active;
    float width;
    float height;
    float damage;
    AEVec2 pos;
    AEVec2 vel;
};

void PlayerBullet_Init(PlayerBullet& bullet, const Player& player); 
void PlayerBullet_Update(PlayerBullet& bullet, float dt);
void PlayerBullet_Draw(const PlayerBullet& bullet);
void PlayerBullet_Free();
void PlayerBullet_Free();

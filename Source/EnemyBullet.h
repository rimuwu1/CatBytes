/* Start Header ************************************************************************/
/*!
\file EnemyBullet.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
\date January, 24, 2026
\brief 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "pch.h"
#include "AEEngine.h" //for AEVec2
#include <memory>
#include "SpriteSheet.h"

const float BULLET_WIDTH = 30.0f;
const float BULLET_HEIGHT = 10.0f;

struct EnemyBullet
{
    AEVec2 pos;
    AEVec2 startPos;
    int direction;//1 = right, -1 = left
    float speed;
    float damage;
    float maxRange;
    bool active;
    float width = BULLET_WIDTH;
    float height = BULLET_HEIGHT;
    std::unique_ptr<SpriteSheet> bulletSprite;
};

void EnemyBullet_Draw(const EnemyBullet& bullet);
void EnemyBullet_Free();

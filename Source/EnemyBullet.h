#pragma once
#include "pch.h"
#include "AEEngine.h" //for AEVec2

struct EnemyBullet
{
    AEVec2 pos;
    AEVec2 startPos;
    int direction;//1 = right, -1 = left
    float speed;
    float damage;
    float maxRange;
    bool active;
};

void EnemyBullet_Draw(const EnemyBullet& bullet);
void EnemyBullet_Free();

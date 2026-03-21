#include "pch.h"
#include "bossroom.h"
#include "ObjectManager.h"
#include "CollisionManager.h"
#include "enemy.h"

void BossRoom::Load()
{
}

void BossRoom::Initialize()
{
}

void BossRoom::Update(float dt)
{
    (void)dt;
    Player& player = ObjectManager::Get().GetPlayer();
    auto& enemies  = ObjectManager::Get().GetAllEnemies();
    for (auto& e : enemies)
    {
        if (e.type == EnemyType::Boss)
            CollisionManager::HandleBossLaserCollisions(player, e);
    }
}

void BossRoom::Draw()
{
    // Draw boss laser telegraphs and beams for any boss in the room
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    for (const auto& e : enemies)
    {
        if (e.type == EnemyType::Boss)
            BossLasers_Draw(e);
    }
}

void BossRoom::Free()
{
}

void BossRoom::Unload()
{
}

/* Start Header ************************************************************************/
/*!
\file       ObjectManager.cpp
\author     Joash ng, joash.ng, 2502780
            Tse Xuan Qi Tristin, tse.x, 2503757
\par        joash.ng@digipen.edu
            tse.x@digipen.edu
\date       Feb 26 2026
\brief		This file handles all the dynamic objects under the object class including player, enemy, boss.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "ObjectManager.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "PlayerBullet.h"
#include "EnemyBullet.h"

// ------------------------------------------------------------------------
// Helper: construct and push a single enemy from a JSON value.
// Shared by LoadFromConfig and AddEnemyFromJSON.
// ------------------------------------------------------------------------
void ObjectManager::AddEnemyFromJSON(const rapidjson::Value& enemyData)
{

    std::string type = enemyData["type"].GetString();
    Enemy newEnemy{};
    
    //d
    if (type == "easy") {
        Enemy_Init(newEnemy, enemyData);
    }
    else if (type == "hard") {
        HardEnemy_Init(newEnemy, enemyData);
    }
    else if (type == "boss") {
        BossEnemy_Init(newEnemy, enemyData);
    }

    //add to enemy list
    enemies.push_back(std::move(newEnemy));
}

// ------------------------------------------------------------------------
// LoadFromConfig  — takes the full document, owns all level traversal.
// Re-initialises player and rebuilds the enemy list from scratch.
// Safe to call on restart without re-loading textures.
// ------------------------------------------------------------------------
void ObjectManager::LoadFromConfig(const rapidjson::Document& doc)
{
    enemies.clear();
    enemyBullets.clear();

    // Player comes from the top-level "player" block
    Player_Init(player, doc["player"]);

    // Gather enemies from every level key present in the document
    // Load enemies from all levels
    const char* levelKeys[] = { "level_1", "level_2", "level_3", "level_4" };

    for (const char* key : levelKeys)
    {
        if (!doc.HasMember(key))
            continue;

        const auto& level = doc[key];

        if (!level.HasMember("enemies") || !level["enemies"].IsArray())
            continue;

        for (const auto& e : level["enemies"].GetArray())
        {
            AddEnemyFromJSON(e);
        }
    }
}

// ------------------------------------------------------------------------
// Initialize  — one-time setup that must only run at startup, not on reset.
// Currently a no-op for ObjectManager (player/enemies are data-driven),
// but kept for symmetry with EnvironmentManager and future expansion.
// ------------------------------------------------------------------------
void ObjectManager::Initialize()
{
    // Nothing needed yet — all state comes from LoadFromConfig.
    // Add audio init, particle system setup, etc. here if required.
}

// ------------------------------------------------------------------------
void ObjectManager::Update(float dt)
{
    Player_Update(player, dt);

    for (auto& e : enemies) {
        //if (!e.isAlive) continue;
        switch (e.type) {
        case EnemyType::Easy: Enemy_Update(e, dt);     break;
        case EnemyType::Hard: HardEnemy_Update(e, dt); break;
        case EnemyType::Boss: BossEnemy_Update(e, dt); break;
        }
    }

    for (auto& b : enemyBullets) {
        if (!b.active) continue;
        b.pos.x += b.direction * b.speed * dt;
        if (fabs(b.pos.x - b.startPos.x) >= b.maxRange)
            b.active = false;
    }

    RemoveInactiveBullets();
}

// ------------------------------------------------------------------------
void ObjectManager::Draw()
{
    for (const auto& e : enemies) {
        //if (e.isAlive) 
         Enemy_Draw(e);
    }
    for (const auto& b : enemyBullets) {
        if (b.active) EnemyBullet_Draw(b);
    }
    Player_Draw(player);
}

// ------------------------------------------------------------------------
void ObjectManager::SpawnEnemyBullet(const Enemy& source, float speed, float damage, float maxRange)
{
    EnemyBullet bullet{};
    bullet.pos = source.pos;
    bullet.startPos = source.pos;
    bullet.direction = source.direction;
    bullet.speed = speed;
    bullet.damage = damage;
    bullet.maxRange = maxRange;
    bullet.active = true;
    enemyBullets.push_back(bullet);
}

// ------------------------------------------------------------------------
bool ObjectManager::IsBossDefeated() const
{
    bool bossExists = false;
    bool bossDead = true;
    for (const auto& e : enemies) {
        if (e.type == EnemyType::Boss) {
            bossExists = true;
            if (e.isAlive) bossDead = false;
        }
    }
    return bossExists && bossDead;
}

// ------------------------------------------------------------------------
void ObjectManager::RemoveInactiveBullets()
{
    enemyBullets.erase(
        std::remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](const EnemyBullet& b) { return !b.active; }),
        enemyBullets.end());
}

// ------------------------------------------------------------------------
void ObjectManager::Clear()
{
    enemies.clear();
    enemyBullets.clear();
}
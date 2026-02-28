/* Start Header ************************************************************************/
/*!
\file       ObjectManager.cpp
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
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
#include "EnemyBullet.h"   // for EnemyBullet_Draw

extern rapidjson::Document configDoc;   // global config loaded once

ObjectManager& ObjectManager::Get() {
    static ObjectManager instance;
    return instance;
}

void ObjectManager::AddEnemyFromJSON(const rapidjson::Value& enemyData) {
    std::string type = enemyData["type"].GetString();
    Enemy newEnemy;

    if (type == "easy") {
        Enemy_Init(newEnemy, enemyData);
        newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/easyenemy.jpg");
        newEnemy.lowHpTexture = TextureManager::Get().LoadTexture("Assets/Images/LowHpOverlay.jpg");
    }
    else if (type == "hard") {
        HardEnemy_Init(newEnemy, enemyData);
        newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/hardenemy.jpg");
        newEnemy.attackTexture = TextureManager::Get().LoadTexture("Assets/Images/HardEnemyAttack.jpg");
        newEnemy.lowHpTexture = TextureManager::Get().LoadTexture("Assets/Images/LowHpOverlay.jpg");
    }
    else if (type == "boss") {
        BossEnemy_Init(newEnemy, enemyData);
        newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/Boss.jpg");
        // Boss may not use low?HP overlay; leave lowHpTexture null
    }

    newEnemy.normalTexture = newEnemy.texture;
    enemies.push_back(newEnemy);
}

void ObjectManager::LoadFromJSON(const rapidjson::Value& levelData) {
    // Clear previous state
    enemies.clear();
    enemyBullets.clear();

    // --- Player: use global config (which contains x,y, stats) ---
    Player_Init(player, configDoc["player"]);

    // --- Enemies ---
    if (levelData.HasMember("enemies") && levelData["enemies"].IsArray()) {
        const auto& enemiesJson = levelData["enemies"];
        for (rapidjson::SizeType i = 0; i < enemiesJson.Size(); ++i) {
            const auto& e = enemiesJson[i];
            std::string type = e["type"].GetString();
            Enemy newEnemy;

            if (type == "easy") {
                Enemy_Init(newEnemy, e);
                newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/easyenemy.jpg");
                newEnemy.lowHpTexture = TextureManager::Get().LoadTexture("Assets/Images/LowHpOverlay.jpg");
            }
            else if (type == "hard") {
                HardEnemy_Init(newEnemy, e);
                newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/hardenemy.jpg");
                newEnemy.attackTexture = TextureManager::Get().LoadTexture("Assets/Images/HardEnemyAttack.jpg");
                newEnemy.lowHpTexture = TextureManager::Get().LoadTexture("Assets/Images/LowHpOverlay.jpg");
            }
            else if (type == "boss") {
                BossEnemy_Init(newEnemy, e);
                newEnemy.texture = TextureManager::Get().LoadTexture("Assets/Images/Boss.jpg");
            }

            newEnemy.normalTexture = newEnemy.texture;   // for hard enemy to revert
            enemies.push_back(newEnemy);
        }
    }
}

void ObjectManager::Update(float dt) {
    Player_Update(player, dt);

    for (auto& e : enemies) {
        if (!e.isAlive) continue;
        switch (e.type) {
        case EnemyType::Easy:  Enemy_Update(e, dt); break;
        case EnemyType::Hard:  HardEnemy_Update(e, dt); break;
        case EnemyType::Boss:  BossEnemy_Update(e, dt); break;
        }
    }

    for (auto& b : enemyBullets) {
        if (!b.active) continue;
        b.pos.x += b.direction * b.speed * dt;
        if (fabs(b.pos.x - b.startPos.x) >= b.maxRange)
            b.active = false;
    }

    RemoveDeadEnemies();
    RemoveInactiveBullets();
}

void ObjectManager::Draw() {
    Player_Draw(player);
    for (const auto& e : enemies) {
        if (e.isAlive) Enemy_Draw(e);
    }
    for (const auto& b : enemyBullets) {
        if (b.active) EnemyBullet_Draw(b);
    }
}

void ObjectManager::SpawnEnemyBullet(const Enemy& source, float speed, float damage, float maxRange) {
    EnemyBullet bullet;
    bullet.pos = source.pos;
    bullet.startPos = source.pos;
    bullet.direction = source.direction;
    bullet.speed = speed;
    bullet.damage = damage;
    bullet.maxRange = maxRange;
    bullet.active = true;
    enemyBullets.push_back(bullet);
}

void ObjectManager::RemoveDeadEnemies() {
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e) { return !e.isAlive; }), enemies.end());
}

void ObjectManager::RemoveInactiveBullets() {
    enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
        [](const EnemyBullet& b) { return !b.active; }), enemyBullets.end());
}
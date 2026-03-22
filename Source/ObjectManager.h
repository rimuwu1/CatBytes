/* Start Header ************************************************************************/
/*!
\file       ObjectManager.h
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date       Feb 26 2026
\brief		This file declares all the dynamic objects functions under the object class including player, enemy, boss

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "Player.h"
#include "enemy.h"
#include "EnemyBullet.h"
#include "PlayerBullet.h"
#include "Buff.h"
#include "MeshManager.h"
#include <vector>
#include <rapidjson/document.h>

class ObjectManager {
public:
    static ObjectManager& Get() {
        static ObjectManager instance;
        return instance;
    }

    // Clears current objects and creates new ones from the given level JSON data
    void LoadFromConfig(const rapidjson::Document&);
    void AddEnemyFromJSON(const rapidjson::Value& enemyData);
    void AddBuffFromJSON(const rapidjson::Value& buffData);
    void Initialize();

    void Update(float dt);
    void Draw(float camX, float camY, float screenHalfW = 800.0f, float screenHalfH = 450.0f);

    // Accessors for collision and other systems
    Player& GetPlayer() { return player; }
    float GetPlayerHP() const { return player.hp; }
    const std::vector<Enemy>& GetAllEnemies() const { return enemies; }
    const std::vector<EnemyBullet>& GetAllEnemyBullets() const { return enemyBullets; }
    const std::vector<Buff>& GetAllBuffs() const { return buffs; }

    // Setters
    std::vector<Enemy>& GetAllEnemies() { return enemies; }
    std::vector<EnemyBullet>& GetAllEnemyBullets() { return enemyBullets; }


    // Bullet spawning for enemies
    void SpawnEnemyBullet(const Enemy& source, float speed, float damage, float maxRange);

    // Buff spawning
    void SpawnBuff(BuffType type, float x, float y);

    //check boss status
    bool IsBossDefeated() const;

    // Rebuild spatial grid with current object positions (call before collision checks)
    void RebuildSpatialGrid();

    // Cleanup helpers
    void RemoveInactiveBullets();
    void RemoveInactiveBuffs();
    void Clear();   // clears enemies and bullets

private:
    ObjectManager() = default;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<EnemyBullet> enemyBullets;
    std::vector<Buff> buffs;
};
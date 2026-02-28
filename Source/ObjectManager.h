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
#include <vector>
#include <rapidjson/document.h>

class ObjectManager {
public:
    static ObjectManager& Get() {
        static ObjectManager instance;
        return instance;
    }

    // Clears current objects and creates new ones from the given level JSON data
    void LoadFromJSON(const rapidjson::Value& levelData);
    void AddEnemyFromJSON(const rapidjson::Value& enemyData);

    void Update(float dt);
    void Draw();

    // Accessors for collision and other systems
    Player& GetPlayer() { return player; }
    std::vector<Enemy>& GetAllEnemies() { return enemies; }
    std::vector<EnemyBullet>& GetAllEnemyBullets() { return enemyBullets; }

    // Bullet spawning for enemies
    void SpawnEnemyBullet(const Enemy& source, float speed, float damage, float maxRange);

    //check boss status
    bool IsBossDefeated() const;

    // Cleanup helpers
    void RemoveInactiveBullets();
    void Clear();   // clears enemies and bullets

private:
    ObjectManager() = default;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<EnemyBullet> enemyBullets;
};
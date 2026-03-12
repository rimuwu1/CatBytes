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
#include "EnvironmentManager.h"
#include "Player.h"

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
// LoadFromConfig takes the full document, owns all level traversal.
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
// Initialize one-time setup that must only run at startup, not on reset.
// Currently a no-op for ObjectManager (player/enemies are data-driven),
// but kept for symmetry with EnvironmentManager and future expansion.
// ------------------------------------------------------------------------
void ObjectManager::Initialize()
{
    // Nothing needed yet all state comes from LoadFromConfig.
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
        if (b.bulletSprite)
            b.bulletSprite->Update(dt);
    }

    RemoveInactiveBullets();
}

// ------------------------------------------------------------------------
static bool IsInView(float x, float y, float halfW, float halfH, float camX, float camY) {
    return (x + halfW) >= (camX - halfW) &&
           (x - halfW) <= (camX + halfW) &&
           (y + halfH) >= (camY - halfH) &&
           (y - halfH) <= (camY + halfH);
}

// ------------------------------------------------------------------------
void ObjectManager::Draw(float camX, float camY, float screenHalfW, float screenHalfH)
{
    MeshManager& mm = MeshManager::Get();

    const float CULL_MARGIN = 100.0f;
    float cullL = camX - screenHalfW - CULL_MARGIN;
    float cullR = camX + screenHalfW + CULL_MARGIN;
    float cullT = camY + screenHalfH + CULL_MARGIN;
    float cullB = camY - screenHalfH - CULL_MARGIN;

    auto InView = [&](float x, float y, float halfW, float halfH) {
        return (x + halfW) >= cullL && (x - halfW) <= cullR &&
               (y + halfH) >= cullB && (y - halfH) <= cullT;
    };

    // Collect all enemy sprites into batches (skip boss - draw separately)
    for (const auto& e : enemies) {
        if (!e.isAlive && e.maxHitPoints > 0.0f) continue;
        if (!InView(e.pos.x, e.pos.y, e.width * 0.5f, e.height * 0.5f)) continue;

        if (e.spriteSheet) {
            // Enemy HP bar
            if (e.isAlive && e.maxHitPoints > 0.0f) {
                float hpRatio = e.hitPoints / e.maxHitPoints;
                float hpBarWidth = e.width * 0.8f;
                float hpBarHeight = 6.0f;
                float hpX = e.pos.x;
                float hpY = e.pos.y + e.height * 0.5f + 10.0f;

                mm.DrawSquare(hpX, hpY, hpBarWidth + 6.0f, hpBarHeight + 6.0f, 0, 0, 0, 1.0f);
                mm.DrawSquare(hpX, hpY, hpBarWidth, hpBarHeight, 40, 40, 40, 1.0f);
                mm.DrawSquare(hpX - (hpBarWidth / 2) + (hpBarWidth * hpRatio) * 0.5f, hpY, hpBarWidth * hpRatio, hpBarHeight, 220, 40, 40, 1.0f);
            }
            // Boss: draw directly (unique sprite, no batching benefit)
            if (e.type == EnemyType::Boss) {
                float scaleX;
                if (e.facesLeft)
                    scaleX = (e.direction == 1) ? -e.width : e.width;
                else
                    scaleX = (e.direction == -1) ? -e.width : e.width;

                MeshManager::Get().DrawSpriteSheet(*e.spriteSheet, e.pos.x, e.pos.y, scaleX, e.height);
            }
            else {
                // Regular enemies: batch them
                float scaleX;
                if (e.facesLeft)
                    scaleX = (e.direction == 1) ? -e.width : e.width;
                else
                    scaleX = (e.direction == -1) ? -e.width : e.width;

                mm.BeginBatch(e.spriteSheet->GetTexture(), e.spriteSheet->GetSpriteUVWidth(), e.spriteSheet->GetSpriteUVHeight());
                SpriteBatchItem item{};
                item.x = e.pos.x;
                item.y = e.pos.y;
                item.width = scaleX;
                item.height = e.height;
                item.uvOffsetX = e.spriteSheet->GetUVOffsetX();
                item.uvOffsetY = e.spriteSheet->GetUVOffsetY();
                item.opacity = 1.0f;
                item.rotation = 0.0f;
                mm.QueueSprite(item);

            }
        }
    }

    // Collect enemy bullets into batch
    for (const auto& b : enemyBullets) {
        if (!b.active) continue;
        if (InView(b.pos.x, b.pos.y, b.width * 0.5f, b.height * 0.5f)) {
            if (b.bulletSprite) {
                mm.BeginBatch(b.bulletSprite->GetTexture(), b.bulletSprite->GetSpriteUVWidth(), b.bulletSprite->GetSpriteUVHeight());
                SpriteBatchItem item{};
                item.x = b.pos.x;
                item.y = b.pos.y;
                item.width = b.width;
                item.height = b.height;
                item.uvOffsetX = b.bulletSprite->GetUVOffsetX();
                item.uvOffsetY = b.bulletSprite->GetUVOffsetY();
                item.opacity = 1.0f;
                item.rotation = 0.0f;
                mm.QueueSprite(item);
            }
        }
    }

    // Player: use existing Player_Draw function (includes slash effect)
    Player_Draw(player);

    // Collect player bullets into batch
    for (const auto& b : player.bullets) {
        if (!b.active) continue;
        if (InView(b.pos.x, b.pos.y, b.width * 0.5f, b.height * 0.5f)) {
            if (b.bulletSprite) {
                mm.BeginBatch(b.bulletSprite->GetTexture(), b.bulletSprite->GetSpriteUVWidth(), b.bulletSprite->GetSpriteUVHeight());
                SpriteBatchItem item{};
                item.x = b.pos.x;
                item.y = b.pos.y;
                item.width = b.width;
                item.height = b.height;
                item.uvOffsetX = b.bulletSprite->GetUVOffsetX();
                item.uvOffsetY = b.bulletSprite->GetUVOffsetY();
                item.opacity = 1.0f;
                item.rotation = 0.0f;
                mm.QueueSprite(item);
            }
        }
    }

    // Flush all batches at once
    mm.EndBatch();
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
    bullet.width = source.bulletWidth;
    bullet.height = source.bulletHeight;
    bullet.active = true;
    if (source.bulletSprite) {
        bullet.bulletSprite = std::make_unique<SpriteSheet>(*source.bulletSprite);
        bullet.bulletSprite->Play("fly", true);
    }
    enemyBullets.push_back(std::move(bullet));
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

void ObjectManager::RebuildSpatialGrid()
{
    auto& env = EnvironmentManager::Get();
    SpatialGrid& grid = env.GetSpatialGrid();

    grid.SetWorldBounds(-500.0f, 11200.0f);
    grid.SetCellHeight(100.0f);
    grid.Clear();

    std::vector<Enemy*> enemyPtrs;
    for (auto& e : enemies) {
        if (e.isAlive) enemyPtrs.push_back(&e);
    }

    std::vector<EnemyBullet*> bulletPtrs;
    for (auto& b : enemyBullets) {
        if (b.active) bulletPtrs.push_back(&b);
    }

    grid.Rebuild(
        env.GetLevel1Platforms(),
        env.GetLevel1Obstacles(),
        env.GetCheckpoints(),
        enemyPtrs,
        bulletPtrs
    );

    grid.RebuildAdd(
        env.GetLevel2Platforms(),
        env.GetLevel2Obstacles()
    );

    grid.RebuildAdd(
        env.GetLevel3Platforms(),
        env.GetLevel3Obstacles()
    );

    grid.RebuildAdd(
        env.GetBossPlatforms(),
        {}
    );

    grid.RebuildAdd(
        env.GetWallPlatforms(),
        {}
    );

    grid.RebuildAdd(
        env.GetLevel3WallPlatforms(),
        {}
    );
}
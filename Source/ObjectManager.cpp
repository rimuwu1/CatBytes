/* Start Header ************************************************************************/
/*!
\file       ObjectManager.cpp
\author     Joash ng, joash.ng, 2502780
            Tse Xuan Qi Tristin, tse.x, 2503757
            Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par        joash.ng@digipen.edu
            tse.x@digipen.edu
            kerwinjiajie.wong@digipen.edu
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
#include <algorithm>

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

void ObjectManager::AddBuffFromJSON(const rapidjson::Value& buffData)
{
    BuffType type = BuffType::NONE;
    std::string buffType = buffData["type"].GetString();

    if (buffType == "shield") {
        type = BuffType::SHIELD;
    }
    else if (buffType == "full_hp") {
        type = BuffType::FULL_HP;
    }
    else if (buffType == "dash") {
        type = BuffType::DASH;
    }

    SpawnBuff(type, buffData["x"].GetFloat(), buffData["y"].GetFloat());
}

// Spawns a new buff in the world
void ObjectManager::SpawnBuff(BuffType type, float x, float y)
{
    buffs.push_back(Buff(type, x, y, 50.0f, 50.0f)); // dropped buff spawn size
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
    buffs.clear();

    // Player comes from the top-level "player" block
    Player_Init(player, doc["player"]);

    // Gather enemies and buffs from every level key present in the document
    // Load enemies from all levels
    const char* levelKeys[] = { "level_1", "level_2", "level_3" };

    for (const char* key : levelKeys)
    {
        if (!doc.HasMember(key))
            continue;

        const auto& level = doc[key];

        // Load enemies
        if (level.HasMember("enemies") && level["enemies"].IsArray())
        {
            for (const auto& e : level["enemies"].GetArray())
            {
                AddEnemyFromJSON(e);
            }
        }

        // Load world buffs
        if (level.HasMember("buffs") && level["buffs"].IsArray())
        {
            for (const auto& b : level["buffs"].GetArray())
            {
                AddBuffFromJSON(b);
            }
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
        case EnemyType::Boss: BossEnemy_Update(e, player, dt); break;
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

    for (auto& buff : buffs) {

        if (!buff.active) continue;

        const float dx = fabs(player.pos.x - buff.pos.x);
        const float dy = fabs(player.pos.y - buff.pos.y);
        const float halfWidth = (player.width + buff.width) * 0.5f;
        const float halfHeight = (player.height + buff.height) * 0.5f;

        if (dx < halfWidth && dy < halfHeight) {
            BuffType collectedType = buff.type;
            Player_PickupBuff(player, buff);
            EnvironmentManager::Get().GetHUD().AddBuffToInventory(collectedType);
        }
    }

    RemoveInactiveBullets();
    RemoveInactiveBuffs();
}

// ------------------------------------------------------------------------
void ObjectManager::Draw(float camX, float camY, float screenHalfW, float screenHalfH)
{
    MeshManager& mm = MeshManager::Get();

    // Culling setup
    const float CULL_MARGIN = 100.0f;
    float cullL = camX - screenHalfW - CULL_MARGIN;
    float cullR = camX + screenHalfW + CULL_MARGIN;
    float cullT = camY + screenHalfH + CULL_MARGIN;
    float cullB = camY - screenHalfH - CULL_MARGIN;

    auto InView = [&](float x, float y, float halfW, float halfH) {
        return (x + halfW) >= cullL && (x - halfW) <= cullR &&
            (y + halfH) >= cullB && (y - halfH) <= cullT;
        };

    // --------------------------------------------------------------------
    // 1. Collect all visible sprites into a single list
    // --------------------------------------------------------------------
    // TODO: move to member
    struct QueuedSprite {
        AEGfxTexture* texture;
        float uvW, uvH;
        float x, y, w, h, uvOffX, uvOffY, opacity, rotation;
        int layer;      // 0 = enemies, 1 = enemy bullets, 2 = player bullets
    };
    std::vector<QueuedSprite> sprites;

    // ----- Enemies (excluding boss) -----
    for (const auto& e : enemies) {
        if (!e.isAlive) continue;
        if (e.type == EnemyType::Boss) {
            // Boss drawn separately later (unique sprite, no batching)
            continue;
        }
        if (!InView(e.pos.x, e.pos.y, e.width * 0.5f, e.height * 0.5f)) continue;
        if (!e.spriteSheet) continue;

        float scaleX;
        if (e.facesLeft)
            scaleX = (e.direction == 1) ? -e.width : e.width;
        else
            scaleX = (e.direction == -1) ? -e.width : e.width;

        QueuedSprite qs;
        qs.texture = e.spriteSheet->GetTexture();
        qs.uvW = e.spriteSheet->GetSpriteUVWidth();
        qs.uvH = e.spriteSheet->GetSpriteUVHeight();
        qs.x = e.pos.x;
        qs.y = e.pos.y;
        qs.w = scaleX;
        qs.h = e.height;
        qs.uvOffX = e.spriteSheet->GetUVOffsetX();
        qs.uvOffY = e.spriteSheet->GetUVOffsetY();
        qs.opacity = 1.0f;
        qs.rotation = 0.0f;
        qs.layer = 0;
        sprites.push_back(qs);
    }

    // ----- Enemy bullets -----
    for (const auto& b : enemyBullets) {
        if (!b.active) continue;
        if (!InView(b.pos.x, b.pos.y, b.width * 0.5f, b.height * 0.5f)) continue;
        if (!b.bulletSprite) continue;

        QueuedSprite qs;
        qs.texture = b.bulletSprite->GetTexture();
        qs.uvW = b.bulletSprite->GetSpriteUVWidth();
        qs.uvH = b.bulletSprite->GetSpriteUVHeight();
        qs.x = b.pos.x;
        qs.y = b.pos.y;
        qs.w = b.width;
        qs.h = b.height;
        qs.uvOffX = b.bulletSprite->GetUVOffsetX();
        qs.uvOffY = b.bulletSprite->GetUVOffsetY();
        qs.opacity = 1.0f;
        qs.rotation = 0.0f;
        qs.layer = 1;
        sprites.push_back(qs);
    }

    // ----- Player bullets -----
    for (const auto& b : player.bullets) {
        if (!b.active) continue;
        if (!InView(b.pos.x, b.pos.y, b.width * 0.5f, b.height * 0.5f)) continue;
        if (!b.bulletSprite) continue;

        QueuedSprite qs;
        qs.texture = b.bulletSprite->GetTexture();
        qs.uvW = b.bulletSprite->GetSpriteUVWidth();
        qs.uvH = b.bulletSprite->GetSpriteUVHeight();
        qs.x = b.pos.x;
        qs.y = b.pos.y;
        qs.w = b.width;
        qs.h = b.height;
        qs.uvOffX = b.bulletSprite->GetUVOffsetX();
        qs.uvOffY = b.bulletSprite->GetUVOffsetY();
        qs.opacity = 1.0f;
        qs.rotation = 0.0f;
        qs.layer = 2;
        sprites.push_back(qs);
    }

    // ----- Buffs -----
    for (const auto& buff : buffs) {
        if (!buff.active) continue;
        if (InView(buff.pos.x, buff.pos.y, buff.width * 0.5f, buff.height * 0.5f))
            buff.Draw(camX, camY);
    }

    // --------------------------------------------------------------------
    // 2. Sort sprites by (layer, texture, uvW, uvH)
    //    This groups sprites into batches while preserving layer order.
    // --------------------------------------------------------------------
    std::sort(sprites.begin(), sprites.end(),
        [](const QueuedSprite& a, const QueuedSprite& b) {
            if (a.layer != b.layer) return a.layer < b.layer;
            if (a.texture != b.texture) return a.texture < b.texture;
            if (a.uvW != b.uvW) return a.uvW < b.uvW;
            return a.uvH < b.uvH;
        });

    // --------------------------------------------------------------------
    // 3. Draw sprites in batches
    // --------------------------------------------------------------------
    size_t i = 0;
    while (i < sprites.size()) {
        const QueuedSprite& first = sprites[i];
        mm.BeginBatch(first.texture, first.uvW, first.uvH);

        // Queue all sprites with same texture and UV dimensions
        do {
            SpriteBatchItem item;
            item.x = sprites[i].x;
            item.y = sprites[i].y;
            item.width = sprites[i].w;
            item.height = sprites[i].h;
            item.uvOffsetX = sprites[i].uvOffX;
            item.uvOffsetY = sprites[i].uvOffY;
            item.opacity = sprites[i].opacity;
            item.rotation = sprites[i].rotation;
            mm.QueueSprite(item);
            ++i;
        } while (i < sprites.size() &&
            sprites[i].layer == first.layer &&
            sprites[i].texture == first.texture &&
            sprites[i].uvW == first.uvW &&
            sprites[i].uvH == first.uvH);

        mm.EndBatch();
    }

    // --------------------------------------------------------------------
    // 4. Draw boss and HP bars (immediate, not batched)
    // --------------------------------------------------------------------
    for (auto& e : enemies) {
        /*if (!e.isAlive) continue;*/
        if (!e.isAlive && e.type != EnemyType::Boss) continue;
        if (e.type == EnemyType::Boss && InView(e.pos.x, e.pos.y, e.width * 0.5f, e.height * 0.5f)) {
            float scaleX;
            if (e.facesLeft)
                scaleX = (e.direction == 1) ? -e.width : e.width;
            else
                scaleX = (e.direction == -1) ? -e.width : e.width;
            MeshManager::Get().DrawSpriteSheet(*e.spriteSheet, e.pos.x, e.pos.y, scaleX, e.height);
        }

        // Enemy HP bars (drawn after boss so they appear on top)
        if (e.isAlive && e.maxHitPoints > 0.0f) {
            float hpRatio = e.hitPoints / e.maxHitPoints;
            float hpBarWidth = e.width * 0.8f;
            float hpBarHeight = 6.0f;
            float hpX = e.pos.x;
            float hpY = e.pos.y + e.height * 0.5f + 10.0f;

            /*if (hpRatio <= 0.0f) {
                e.isAlive = false;
            }*/

            mm.DrawSquare(hpX, hpY, hpBarWidth + 6.0f, hpBarHeight + 6.0f, 0, 0, 0, 1.0f);
            mm.DrawSquare(hpX, hpY, hpBarWidth, hpBarHeight, 40, 40, 40, 1.0f);
            mm.DrawSquare(hpX - (hpBarWidth / 2) + (hpBarWidth * hpRatio) * 0.5f, hpY, hpBarWidth * hpRatio, hpBarHeight, 220, 40, 40, 1.0f);
        }
    }

    // --------------------------------------------------------------------
    // 5. Draw player (including slash effect)
    // --------------------------------------------------------------------
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

void ObjectManager::RemoveInactiveBuffs()
{
    buffs.erase(
        std::remove_if(buffs.begin(), buffs.end(),
            [](const Buff& buff) { return !buff.active; }),
        buffs.end());
}

// ------------------------------------------------------------------------
void ObjectManager::Clear()
{
    enemies.clear();
    enemyBullets.clear();
    buffs.clear();
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
        bulletPtrs,
        buffs
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

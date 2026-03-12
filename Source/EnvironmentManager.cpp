/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.cpp
\author     Joash ng, joash.ng, 2502780
            Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
            Sim Hui Min, s.huimin, 2503506
\par        joash.ng@digipen.edu
            p.yuxuanlovette@digipen.edu
            s.huimin@digipen.edu
\date       Feb 26 2026
\brief		This file handles all the environment stuff like platforms obstacles and walls.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "EnvironmentManager.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include "LevelIndicator.h"
#include "GameSaveManager.h"
#include "Player.h"
#include "enemy.h"
#include "EnemyBullet.h"
#include "ObjectManager.h"
#include "rapidjson/document.h"
#include <algorithm>

// ------------------------------------------------------------------------
// Helper: parse a platform array from a JSON value into a vector
// ------------------------------------------------------------------------
static void ParsePlatforms(const rapidjson::Value& arr, std::vector<Platform>& out)
{
    out.clear();
    for (const auto& p : arr.GetArray()) {
        Platform pf{};
        pf.x = p["x"].GetFloat();
        pf.y = p["y"].GetFloat();
        pf.w = p["width"].GetFloat();
        pf.h = p["height"].GetFloat();
        pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
        out.push_back(pf);
    }
}

// ------------------------------------------------------------------------
// LoadFromConfig  (takes the full document, owns all traversal internally)
// ------------------------------------------------------------------------
void EnvironmentManager::LoadFromConfig(const rapidjson::Document& doc)
{
    // ---- HUD ----
    if (doc.HasMember("ui"))
        m_HUD.InitFromConfig(doc);

    // ---- Button configuration (must be read before creating buttons) ----
    if (doc.HasMember("button") && doc["button"].IsObject()) {
        const auto& btnCfg = doc["button"];
        m_buttonFilePath = btnCfg["file"].GetString();
        m_buttonRows = btnCfg["rows"].GetInt();
        m_buttonCols = btnCfg["cols"].GetInt();
        m_buttonTotalFrames = btnCfg["total_frames"].GetInt();
        m_buttonFrameDuration = btnCfg["frame_duration"].GetFloat();

        m_buttonClips.clear();
        for (const auto& clip : btnCfg["clips"].GetArray()) {
            ButtonClipConfig cfg;
            cfg.name = clip["name"].GetString();
            cfg.start = clip["start"].GetInt();
            cfg.end = clip["end"].GetInt();
            cfg.duration = clip["duration"].GetFloat();
            cfg.loop = clip["loop"].GetBool();
            m_buttonClips.push_back(cfg);
        }
    }
    else {
        // Fallback defaults (optional – prevents crash if config missing)
        m_buttonFilePath = "Assets/Images/buttonSheet2.png";
        m_buttonRows = 3;
        m_buttonCols = 4;
        m_buttonTotalFrames = 9;
        m_buttonFrameDuration = 0.1f;
        m_buttonClips = {
            {"off", 8, 8, 0.1f, false},
            {"transition", 0, 3, 0.5f, false},
            {"on", 4, 7, 0.1f, true}
        };
    }

    // ---- Platforms per level ----
    const struct { const char* key; std::vector<Platform>* target; } platformMaps[] = {
        { "level_1", &m_level1Platforms },
        { "level_2", &m_level2Platforms },
        { "level_3", &m_level3Platforms },
        { "level_4", &m_bossPlatforms   },
    };
    for (const auto& entry : platformMaps) {
        if (doc.HasMember(entry.key) && doc[entry.key].HasMember("platforms"))
            ParsePlatforms(doc[entry.key]["platforms"], *entry.target);
    }

    // ---- Walls (level_1 only) ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("walls")) {
        m_wallPlatforms.clear();
        for (const auto& w : doc["level_1"]["walls"].GetArray()) {
            Platform wall{};
            wall.x = w["x"].GetFloat();
            wall.y = w["y"].GetFloat();
            wall.w = w["width"].GetFloat();
            wall.h = w["height"].GetFloat();
            wall.active = true;   // walls are always active
            m_wallPlatforms.push_back(wall);
        }
    }

    // ---- Level 1 buttons ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("buttons")) {
        m_level1Buttons.clear();
        for (const auto& b : doc["level_1"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;

            // --- Create per?button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            m_level1Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_1 only) ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("obstacles")) {
        m_level1Obstacles.clear();
        for (const auto& o : doc["level_1"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true;
            m_level1Obstacles.push_back(obs);
        }
    }

    // ---- Level 2 buttons ----
    if (doc.HasMember("level_2") && doc["level_2"].HasMember("buttons")) {
        m_level2Buttons.clear();
        for (const auto& b : doc["level_2"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;
            // --- Create per?button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            m_level2Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_2 only) ----
    if (doc.HasMember("level_2") && doc["level_2"].HasMember("obstacles")) {
        m_level2Obstacles.clear();
        for (const auto& o : doc["level_2"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true; 
            m_level2Obstacles.push_back(obs);
        }
    }

    // ---- Walls (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("walls")) {
        m_level3WallPlatforms.clear();
        for (const auto& w : doc["level_3"]["walls"].GetArray()) {
            Platform wall{};
            wall.x = w["x"].GetFloat();
            wall.y = w["y"].GetFloat();
            wall.w = w["width"].GetFloat();
            wall.h = w["height"].GetFloat();
            wall.active = true;   // walls are always active
            m_level3WallPlatforms.push_back(wall);
        }
    }

    // ---- Level 3 buttons ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("buttons")) {
        m_level3Buttons.clear();
        for (const auto& b : doc["level_3"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;
            // --- Create per?button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            m_level3Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("obstacles")) {
        m_level3Obstacles.clear();
        for (const auto& o : doc["level_3"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true; 
            m_level3Obstacles.push_back(obs);
        }
    }

    // ---- Checkpoints ----
    if (doc.HasMember("checkpoints")) {
        m_checkpoints.clear();
        for (const auto& pt : doc["checkpoints"].GetArray()) {
            Checkpoint cp{};
            cp.x = pt["x"].GetFloat();
            cp.y = pt["y"].GetFloat();
            cp.w = pt["width"].GetFloat();
            cp.h = pt["height"].GetFloat();
            m_checkpoints.push_back(cp);
        }
    }

    // Reset runtime state so a reload/restart starts clean
    m_checkpointSaved = false;
    m_saveRequested = false;
}

// ------------------------------------------------------------------------
// Initialize textures and one-time setup only, safe to call once at startup
// ------------------------------------------------------------------------
void EnvironmentManager::Initialize()
{
    // Background initial colour
    m_currentColour = m_backgroundColours[0];
    m_previousSelection = -1;

    // Load platform textures
    m_leftTex = TextureManager::Get().LoadTexture("Assets/Images/platform_left.png");
    m_midTex = TextureManager::Get().LoadTexture("Assets/Images/platform_middle.png");
    m_rightTex = TextureManager::Get().LoadTexture("Assets/Images/platform_right.png");

    m_hoverAnim = std::make_unique<SpriteSheet>("Assets/Images/HoverSheet.png", 1, 4, 0, .1f);
    m_checkpointAnim = std::make_unique<SpriteSheet>("Assets/Images/checkpointSheet.png", 1, 10, 0, 0.1f);
    m_spikeTex = TextureManager::Get().LoadTexture("Assets/Images/spikeObstacle.png");

    // Level indicator initialise (free function)
    LevelIndicator_Initialize();
}

// ------------------------------------------------------------------------
void EnvironmentManager::Update(float dt, const Player& player, float cameraY)
{
    m_HUD.Update(dt, player, player.weapon);

    m_hoverAnim->Update(dt);
    m_checkpointAnim->Update(dt);
    // m_buttonAnim is not animated in this simplified version

    UpdateBackground(cameraY);

    int section = GetSectionFromY(cameraY);
    if (section != m_previousSelection) {
        LevelIndicator_Show(section);
        m_previousSelection = section;
    }
    LevelIndicator_Update(dt);
}

// ------------------------------------------------------------------------
void EnvironmentManager::Draw(float camX, float camY, PlayerWeapon weapon, const Player& player, float screenHalfH)
{
    MeshManager& mm = MeshManager::Get();

    DrawBackground();

    const float CULL_MARGIN = 200.0f;
    const float cullHalf = screenHalfH + CULL_MARGIN;

    auto inView = [&](float objY, float halfH) -> bool {
        return (objY + halfH) >= (camY - cullHalf) &&
            (objY - halfH) <= (camY + cullHalf);
        };

    const float capWidth = 32.0f; // width of platform caps

    // --------------------------------------------------------------------
    // Helper structure for queued sprites
    struct QueuedSprite {
        AEGfxTexture* texture;
        float uvW, uvH;
        float x, y, w, h;
        float uvOffX, uvOffY;
        float opacity;
        float rotation;
    };
    std::vector<QueuedSprite> sprites;

    auto addSprite = [&](AEGfxTexture* tex, float uvW, float uvH,
        float x, float y, float w, float h,
        float uvOffX, float uvOffY,
        float opacity = 1.0f, float rotation = 0.0f) {
            sprites.push_back({ tex, uvW, uvH, x, y, w, h, uvOffX, uvOffY, opacity, rotation });
        };

    // Helper to sort and draw a batch
    auto flushBatch = [&]() {
        if (sprites.empty()) return;
        std::sort(sprites.begin(), sprites.end(),
            [](const QueuedSprite& a, const QueuedSprite& b) {
                if (a.texture != b.texture) return a.texture < b.texture;
                if (a.uvW != b.uvW) return a.uvW < b.uvW;
                return a.uvH < b.uvH;
            });

        size_t i = 0;
        while (i < sprites.size()) {
            const QueuedSprite& first = sprites[i];
            mm.BeginBatch(first.texture, first.uvW, first.uvH);
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
                sprites[i].texture == first.texture &&
                sprites[i].uvW == first.uvW &&
                sprites[i].uvH == first.uvH);
            mm.EndBatch();
        }
        sprites.clear();
        };

    // --------------------------------------------------------------------
    // 1. Platforms (caps, middle, hover)
    // --------------------------------------------------------------------
    auto collectPlatforms = [&](const std::vector<Platform>& platforms) {
        for (const auto& p : platforms) {
            if (!p.active) continue;
            if (!inView(p.y, p.h * 0.5f)) continue;

            float leftX = p.x - p.w * 0.5f + capWidth * 0.5f;
            float rightX = p.x + p.w * 0.5f - capWidth * 0.5f;

            // Left cap
            addSprite(m_leftTex, 1.0f, 1.0f, leftX, p.y, capWidth, p.h, 0.0f, 0.0f);
            // Right cap
            addSprite(m_rightTex, 1.0f, 1.0f, rightX, p.y, capWidth, p.h, 0.0f, 0.0f);

            float midStartX = leftX + capWidth * 0.5f;
            float midEndX = rightX - capWidth * 0.5f;
            float midWidth = midEndX - midStartX;
            if (midWidth > 0.0f) {
                float midCenterX = (midStartX + midEndX) * 0.5f;
                // Middle section
                addSprite(m_midTex, 1.0f, 1.0f, midCenterX, p.y, midWidth, p.h, 0.0f, 0.0f);
                // Hover animation (if available)
                if (m_hoverAnim) {
                    addSprite(m_hoverAnim->GetTexture(),
                        m_hoverAnim->GetSpriteUVWidth(),
                        m_hoverAnim->GetSpriteUVHeight(),
                        midCenterX, p.y - 40.0f, midWidth, p.h,
                        m_hoverAnim->GetUVOffsetX(),
                        m_hoverAnim->GetUVOffsetY());
                }
            }
        }
        };

    collectPlatforms(m_level1Platforms);
    collectPlatforms(m_level2Platforms);
    collectPlatforms(m_level3Platforms);
    collectPlatforms(m_bossPlatforms);
    collectPlatforms(m_wallPlatforms);
    collectPlatforms(m_level3WallPlatforms);

    // --------------------------------------------------------------------
    // 2. Obstacles (spikes)
    // --------------------------------------------------------------------
    auto collectObstacles = [&](const std::vector<PlatformObstacle>& obstacles) {
        for (const auto& o : obstacles) {
            if (!inView(o.y, o.h * 0.5f)) continue;
            addSprite(m_spikeTex, 1.0f, 1.0f, o.x, o.y, o.w, o.h, 0.0f, 0.0f);
        }
        };

    collectObstacles(m_level1Obstacles);
    collectObstacles(m_level2Obstacles);
    collectObstacles(m_level3Obstacles);

    //// --------------------------------------------------------------------
    //// 3. Buttons (static frames, no animation in batch)
    //// --------------------------------------------------------------------
    //auto collectButtons = [&](const std::vector<PlatformButton>& buttons,
    //    const std::vector<Platform>& platforms) {
    //        if (!m_buttonAnim) return;
    //        float uvW = m_buttonAnim->GetSpriteUVWidth();
    //        float uvH = m_buttonAnim->GetSpriteUVHeight();

    //        for (const auto& btn : buttons) {
    //            if (!inView(btn.y, btn.h * 0.5f)) continue;

    //            bool isActive = false;
    //            if (!btn.platformIndices.empty()) {
    //                int idx = btn.platformIndices[0];
    //                if (idx >= 0 && idx < (int)platforms.size())
    //                    isActive = platforms[idx].active;
    //            }

    //            // Off = row2 (index 8), On = row1 (index 4) assuming 4 columns
    //            float uvOffY = isActive ? (1.0f / 3.0f) : (2.0f / 3.0f); // row1 or row2
    //            addSprite(m_buttonAnim->GetTexture(), uvW, uvH,
    //                btn.x, btn.y, btn.w, btn.h,
    //                0.0f, uvOffY);
    //        }
    //    };

    //collectButtons(m_level1Buttons, m_level1Platforms);
    //collectButtons(m_level2Buttons, m_level2Platforms);
    //collectButtons(m_level3Buttons, m_level3Platforms);

    // --------------------------------------------------------------------
    // 4. Checkpoints
    // --------------------------------------------------------------------
    auto collectCheckpoints = [&](const std::vector<Checkpoint>& cps) {
        for (const auto& cp : cps) {
            if (!inView(cp.y, cp.h * 0.5f)) continue;
            if (m_checkpointAnim) {
                addSprite(m_checkpointAnim->GetTexture(),
                    m_checkpointAnim->GetSpriteUVWidth(),
                    m_checkpointAnim->GetSpriteUVHeight(),
                    cp.x, cp.y, cp.w, cp.h,
                    m_checkpointAnim->GetUVOffsetX(),
                    m_checkpointAnim->GetUVOffsetY());
            }
        }
        };

    collectCheckpoints(m_checkpoints);
    flushBatch();

    PlatformButton_Draw(m_level1Buttons, m_level1Platforms, player);
    PlatformButton_Draw(m_level2Buttons, m_level2Platforms, player);
    PlatformButton_Draw(m_level3Buttons, m_level3Platforms, player);

    // --------------------------------------------------------------------
    // 5. Ground (single colored square)
    // --------------------------------------------------------------------
    mm.DrawSquare(0.0f, -350.0f, 1600.0f, 50.0f, 0, 0, 0);

    // --------------------------------------------------------------------
    // 6. HUD and level indicator (use immediate drawing)
    // --------------------------------------------------------------------
    m_HUD.Draw(mm, camX, camY, weapon);
    LevelIndicator_Draw();
}

// ------------------------------------------------------------------------
// Private background helpers
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateBackground(float cameraY)
{
    int index = GetSectionFromY(cameraY);
    m_currentSection = index;

    float lower = (index == 0) ? 0.0f : m_sectionHeights[index - 1];
    float upper = m_sectionHeights[index];
    float blend = (cameraY - lower) / (upper - lower);

    m_currentColour = (index < BACKGROUND_SECTIONS - 1)
        ? BlendColours(m_backgroundColours[index], m_backgroundColours[index + 1], blend)
        : m_backgroundColours[index];
}

void EnvironmentManager::DrawBackground() const
{
    AEGfxSetBackgroundColor(m_currentColour.r, m_currentColour.g, m_currentColour.b);
}

// ------------------------------------------------------------------------
bool EnvironmentManager::HandleCheckpoint(bool checkpointHit)
{
    bool shouldSave = false;

    if (checkpointHit || m_saveRequested)
    {
        if (!m_checkpointSaved || m_saveRequested)
        {
            m_checkpointSaved = true;
            shouldSave = true;
            m_saveRequested = false;
        }
    }
    else
    {
        m_checkpointSaved = false;
    }

    return shouldSave;
}

void EnvironmentManager::RequestSave()
{
    m_saveRequested = true;
}

bool EnvironmentManager::isSaveRequested()
{
    bool shouldSave = false;
    if (m_saveRequested) {
        shouldSave = true;
        m_saveRequested = false; // reset after checking
    }
    return shouldSave;
}

void EnvironmentManager::SetCheckpointInRange(bool inRange)
{
    m_checkpointInRange = inRange;
}

bool EnvironmentManager::GetCheckpointInRange() const
{
    return m_checkpointInRange;
}

// ------------------------------------------------------------------------
int EnvironmentManager::GetSectionFromY(float y) const
{
    for (int i = 0; i < BACKGROUND_SECTIONS; ++i) {
        if (y <= m_sectionHeights[i])
            return i;
    }
    return BACKGROUND_SECTIONS - 1;
}

const std::vector<PlatformObstacle>& EnvironmentManager::GetCurrentObstacles() const
{
    int currentLevel = m_currentSection + 1;

    switch (currentLevel) {
    case 1:  return m_level1Obstacles;
    case 2:  return m_level2Obstacles;
    case 3:  return m_level3Obstacles;
    default: return m_level1Obstacles;
    }
}

EnvironmentManager::Colour EnvironmentManager::BlendColours(const Colour& a, const Colour& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// ------------------------------------------------------------------------
// Level indicator wrappers
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateLevelIndicator(float dt) { LevelIndicator_Update(dt); }
void EnvironmentManager::DrawLevelIndicator()  const { LevelIndicator_Draw(); }

// ------------------------------------------------------------------------
void EnvironmentManager::Clear()
{
    m_level1Platforms.clear();
    m_level2Platforms.clear();
    m_level3Platforms.clear();
    m_bossPlatforms.clear();
    m_wallPlatforms.clear();
    m_level3WallPlatforms.clear();
    m_level1Obstacles.clear();
    m_level2Obstacles.clear();
    m_level3Obstacles.clear();
    m_checkpoints.clear();
    m_level1Buttons.clear();
    m_level2Buttons.clear();
    m_level3Buttons.clear();
}
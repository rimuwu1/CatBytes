/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.cpp
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
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
#include "FileManager.h"
#include "Player.h"
#include "rapidjson/document.h"
#include "AEEngine.h"

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
            btn.platformIndex = b["platformIndex"].GetInt();
            btn.wasPressed = false;
            m_level2Buttons.push_back(btn);
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
// Initialize  — textures and one-time setup only, safe to call once at startup
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

    // Level indicator initialise (free function)
    LevelIndicator_Initialize();
}

// ------------------------------------------------------------------------
void EnvironmentManager::Update(float dt, const Player& player, float cameraY)
{
    m_HUD.Update(dt, player);

    UpdateBackground(cameraY);

    int section = GetSectionFromY(cameraY);
    if (section != m_previousSelection) {
        LevelIndicator_Show(section);
        m_previousSelection = section;
    }
    LevelIndicator_Update(dt);
}

// ------------------------------------------------------------------------
void EnvironmentManager::Draw(float camX, float camY, float /*playerX*/, float /*playerY*/)
{
    DrawBackground();

    Platforms_Draw(m_level1Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_level2Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_level3Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_bossPlatforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_wallPlatforms, m_leftTex, m_midTex, m_rightTex);

    PlatformButton_Draw(m_level2Buttons, m_level2Platforms);
    PlatformsObstacle_Draw(m_level1Obstacles);
    CheckpointDraw(m_checkpoints);

    MeshManager::Get().DrawSquare(0.0f, -350.0f, 1600.0f, 50.0f, 0, 0, 0);

    m_HUD.Draw(MeshManager::Get(), camX, camY);
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

// ------------------------------------------------------------------------
int EnvironmentManager::GetSectionFromY(float y) const
{
    for (int i = 0; i < BACKGROUND_SECTIONS; ++i) {
        if (y <= m_sectionHeights[i])
            return i;
    }
    return BACKGROUND_SECTIONS - 1;
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
    m_level1Obstacles.clear();
    m_checkpoints.clear();
    m_level2Buttons.clear();
}
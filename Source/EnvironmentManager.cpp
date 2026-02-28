/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.cpp
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date       Feb 26 2026
\brief		This file handles all the environment stuff like platforms obstacles and walls .

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
#include "LevelIndicator.h"   // for LevelIndicator_Show, _Update, _Draw
#include "FileManager.h"         // for save notification (optional, not drawn here)
#include "Player.h"
#include "rapidjson/document.h"
#include "AEEngine.h"

// ------------------------------------------------------------------------
void EnvironmentManager::LoadFromConfig(const rapidjson::Value& config)
{
    // ---- HUD ----
    if (config.HasMember("ui"))
        m_HUD.InitFromConfig(config);

    // ---- Level 1 platforms ----
    if (config.HasMember("level_1") && config["level_1"].HasMember("platforms")) {
        const auto& platforms = config["level_1"]["platforms"];
        m_level1Platforms.clear();
        for (auto& p : platforms.GetArray()) {
            Platform pf{};
            pf.x = p["x"].GetFloat();
            pf.y = p["y"].GetFloat();
            pf.w = p["width"].GetFloat();
            pf.h = p["height"].GetFloat();
            pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
            m_level1Platforms.push_back(pf);
        }
    }

    // ---- Level 2 platforms ----
    if (config.HasMember("level_2") && config["level_2"].HasMember("platforms")) {
        const auto& platforms = config["level_2"]["platforms"];
        m_level2Platforms.clear();
        for (auto& p : platforms.GetArray()) {
            Platform pf{};
            pf.x = p["x"].GetFloat();
            pf.y = p["y"].GetFloat();
            pf.w = p["width"].GetFloat();
            pf.h = p["height"].GetFloat();
            pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
            m_level2Platforms.push_back(pf);
        }
    }

    // ---- Level 2 buttons ----
    if (config.HasMember("level_2") && config["level_2"].HasMember("buttons")) {
        const auto& buttons = config["level_2"]["buttons"];
        m_level2Buttons.clear();
        for (auto& b : buttons.GetArray()) {
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

    // ---- Level 3 platforms ----
    if (config.HasMember("level_3") && config["level_3"].HasMember("platforms")) {
        const auto& platforms = config["level_3"]["platforms"];
        m_level3Platforms.clear();
        for (auto& p : platforms.GetArray()) {
            Platform pf{};
            pf.x = p["x"].GetFloat();
            pf.y = p["y"].GetFloat();
            pf.w = p["width"].GetFloat();
            pf.h = p["height"].GetFloat();
            pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
            m_level3Platforms.push_back(pf);
        }
    }

    // ---- Level 4 (boss) platforms ----
    if (config.HasMember("level_4") && config["level_4"].HasMember("platforms")) {
        const auto& platforms = config["level_4"]["platforms"];
        m_bossPlatforms.clear();
        for (auto& p : platforms.GetArray()) {
            Platform pf{};
            pf.x = p["x"].GetFloat();
            pf.y = p["y"].GetFloat();
            pf.w = p["width"].GetFloat();
            pf.h = p["height"].GetFloat();
            pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
            m_bossPlatforms.push_back(pf);
        }
    }

    // ---- Walls ----
    if (config.HasMember("level_1") && config["level_1"].HasMember("walls")) {
        const auto& walls = config["level_1"]["walls"];
        m_wallPlatforms.clear();
        for (auto& w : walls.GetArray()) {
            Platform newWall{};
            newWall.x = w["x"].GetFloat();
            newWall.y = w["y"].GetFloat();
            newWall.w = w["width"].GetFloat();
            newWall.h = w["height"].GetFloat();
            newWall.active = true;   // walls are always active
            m_wallPlatforms.push_back(newWall);
        }
    }

    // ---- Obstacles ----
    if (config.HasMember("level_1") && config["level_1"].HasMember("obstacles")) {
        const auto& obstacles = config["level_1"]["obstacles"];
        m_level1Obstacles.clear();
        for (auto& o : obstacles.GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            m_level1Obstacles.push_back(obs);
        }
    }

    // ---- Checkpoints ----
    if (config.HasMember("checkpoints")) {
        const auto& points = config["checkpoints"];
        m_checkpoints.clear();
        for (auto& pt : points.GetArray()) {
            Checkpoint cp{};
            cp.x = pt["x"].GetFloat();
            cp.y = pt["y"].GetFloat();
            cp.w = pt["width"].GetFloat();
            cp.h = pt["height"].GetFloat();
            m_checkpoints.push_back(cp);
        }
    }
}

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
    // Update HUD
    m_HUD.Update(dt, player);

    // Update background (colour based on camera Y)
    UpdateBackground(cameraY);

    // Update level indicator if section changed
    int section = GetSectionFromY(cameraY);
    if (section != m_previousSelection) {
        LevelIndicator_Show(section);
        m_previousSelection = section;
    }
    LevelIndicator_Update(dt);
}

// ------------------------------------------------------------------------
void EnvironmentManager::Draw(float camX, float camY, float playerX, float playerY)
{
    DrawBackground();

    // Draw all platforms
    Platforms_Draw(m_level1Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_level2Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_level3Platforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_bossPlatforms, m_leftTex, m_midTex, m_rightTex);
    Platforms_Draw(m_wallPlatforms, m_leftTex, m_midTex, m_rightTex);

    // Draw buttons, obstacles, checkpoints
    PlatformButton_Draw(m_level2Buttons, m_level2Platforms);
    PlatformsObstacle_Draw(m_level1Obstacles);
    CheckpointDraw(m_checkpoints);

    // Ground
    MeshManager::Get().DrawSquare(0.0f, -350.0f, 1600.0f, 50.0f, 0, 0, 0);

    // HUD, level indicator
    m_HUD.Draw(MeshManager::Get(), camX, camY);
    LevelIndicator_Draw();
}


// ------------------------------------------------------------------------
// Private background methods
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateBackground(float cameraY)
{
    int index = GetSectionFromY(cameraY);
    m_currentSection = index;

    float lower = (index == 0) ? 0.0f : m_sectionHeights[index - 1];
    float upper = m_sectionHeights[index];
    float blend = (cameraY - lower) / (upper - lower);

    if (index < BACKGROUND_SECTIONS - 1) {
        m_currentColour = BlendColours(m_backgroundColours[index],
            m_backgroundColours[index + 1],
            blend);
    }
    else {
        m_currentColour = m_backgroundColours[index];
    }
}

// ------------------------------------------------------------------------
void EnvironmentManager::DrawBackground() const
{
    AEGfxSetBackgroundColor(m_currentColour.r, m_currentColour.g, m_currentColour.b);
}

// ------------------------------------------------------------------------
bool EnvironmentManager::HandleCheckpoint(bool checkpointHit, bool& externalSaveRequest)
{
    bool shouldSave = false;

    if (checkpointHit || externalSaveRequest || m_saveRequested)
    {
        if (!m_checkpointSaved || externalSaveRequest || m_saveRequested)
        {
            m_checkpointSaved = true;
            shouldSave = true;

            // Consume external request if any
            externalSaveRequest = false;
            m_saveRequested = false;
        }
    }
    else
    {
        m_checkpointSaved = false;  // reset cooldown when no checkpoint/save requested
    }

    return shouldSave;
}

// ------------------------------------------------------------------------
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

// ------------------------------------------------------------------------
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
// Level indicator (calls existing free functions)
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateLevelIndicator(float dt)
{
    LevelIndicator_Update(dt);
}

void EnvironmentManager::DrawLevelIndicator() const
{
    LevelIndicator_Draw();
}

//cleanup
void EnvironmentManager::Clear() {
    m_level1Platforms.clear();
    m_level2Platforms.clear();
    m_level3Platforms.clear();
    m_bossPlatforms.clear();
    m_wallPlatforms.clear();
    m_level1Obstacles.clear();
    m_checkpoints.clear();
    m_level2Buttons.clear();
}
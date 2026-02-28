#pragma once
#include <vector>
#include "HUD.h"
#include "Minimap.h"
#include "Platforms.h"
#include "SpriteSheet.h"
#include "rapidjson/document.h"
#include "AEEngine.h"


class EnvironmentManager {
public:
    void LoadFromConfig(const rapidjson::Value& config);   // expects the whole game config document
    void Initialize();
    void Update(float dt, const Player& player, float cameraY);
    void Draw(float camX, float camY, float playerX, float playerY);
    void Free();
    void Unload();

    // Accessors for collision and gameplay systems
    const std::vector<Platform>& GetLevel1Platforms() const { return m_level1Platforms; }
    const std::vector<Platform>& GetLevel2Platforms() const { return m_level2Platforms; }
    const std::vector<Platform>& GetLevel3Platforms() const { return m_level3Platforms; }
    const std::vector<Platform>& GetBossPlatforms()  const { return m_bossPlatforms; }
    const std::vector<Platform>& GetWallPlatforms()  const { return m_wallPlatforms; }
    const std::vector<PlatformObstacle>& GetObstacles() const { return m_level1Obstacles; }
    const std::vector<Checkpoint>& GetCheckpoints() const { return m_checkpoints; }

    // Non?const access for buttons (they modify platforms)
    std::vector<PlatformButton>& GetLevel2Buttons() { return m_level2Buttons; }
    const std::vector<PlatformButton>& GetLevel2Buttons() const { return m_level2Buttons; }

    // Current background section (0?based)
    int GetCurrentSection() const { return m_currentSection; }

private:
    // Background system (embedded from Background.cpp)
    void UpdateBackground(float cameraY);
    void DrawBackground() const;
    int  GetSectionFromY(float y) const;

    // Level indicator (using existing free functions)
    void UpdateLevelIndicator(float dt);
    void DrawLevelIndicator() const;

    // Platform textures
    AEGfxTexture* m_leftTex = nullptr;
    AEGfxTexture* m_midTex = nullptr;
    AEGfxTexture* m_rightTex = nullptr;

    // Environment data
    HUD      m_HUD;
    Minimap  m_Minimap;

    std::vector<Platform>         m_level1Platforms;
    std::vector<Platform>         m_level2Platforms;
    std::vector<Platform>         m_level3Platforms;
    std::vector<Platform>         m_bossPlatforms;
    std::vector<Platform>         m_wallPlatforms;
    std::vector<PlatformObstacle> m_level1Obstacles;
    std::vector<Checkpoint>       m_checkpoints;
    std::vector<PlatformButton>   m_level2Buttons;

    int m_previousSelection = -1;   // for level indicator
    int m_currentSection = 0;

    // Background constants
    static constexpr int BACKGROUND_SECTIONS = 4;
    float m_sectionHeights[BACKGROUND_SECTIONS] = { 1900.0f, 4550.0f, 7500.0f, 10500.0f };

    struct Colour { float r, g, b, a; };
    Colour m_backgroundColours[BACKGROUND_SECTIONS] = {
        {189 / 255.0f, 231 / 255.0f, 255 / 255.0f, 1.0f},   // level 1 blue
        { 87 / 255.0f, 119 / 255.0f, 165 / 255.0f, 1.0f},   // level 2 dark blue
        {104 / 255.0f,  26 / 255.0f, 163 / 255.0f, 1.0f},   // level 3 purple
        { 43 / 255.0f,   4 / 255.0f,  56 / 255.0f, 1.0f}    // level 4 dark purple
    };
    Colour m_currentColour = m_backgroundColours[0];

    static Colour BlendColours(const Colour& a, const Colour& b, float t);
};
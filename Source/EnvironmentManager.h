/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.h
\author     Joash ng, joash.ng, 2502780
            Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
            Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        joash.ng@digipen.edu
            kerwinjiajie.wong@digipen.edu
            p.yuxuanlovette@digipen.edu
\date       Feb 26 2026
\brief		This file declares rhe Environmanager Namespace which handles all the environment stuff like platforms obstacles and walls through a singleton instance.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include <vector>
#include "HUD.h"
#include "Platforms.h"
#include "SpriteSheet.h"
#include "rapidjson/document.h"
#include "AEEngine.h"


class EnvironmentManager {
public:
    static EnvironmentManager& Get() {
        static EnvironmentManager instance;
        return instance;
    }
    // Delete copy/move constructors and assignment
    EnvironmentManager(const EnvironmentManager&) = delete;
    EnvironmentManager& operator=(const EnvironmentManager&) = delete;

    void LoadFromConfig(const rapidjson::Document&);
    void Initialize();
    void Update(float dt, const Player& player, float cameraY);
    void Draw(float camX, float camY, PlayerWeapon weapon/* float playerX, float playerY*/);

    // Accessors for collision and gameplay systems
    const std::vector<Platform>& GetLevel1Platforms() const { return m_level1Platforms; }
    const std::vector<PlatformButton>& GetLevel1Buttons() const { return m_level1Buttons; }
    const std::vector<PlatformObstacle>& GetLevel1Obstacles() const { return m_level1Obstacles; }

    const std::vector<Platform>& GetLevel2Platforms() const { return m_level2Platforms; }
    const std::vector<PlatformButton>& GetLevel2Buttons() const { return m_level2Buttons; }
    const std::vector<PlatformObstacle>& GetLevel2Obstacles() const { return m_level2Obstacles; }

    const std::vector<Platform>& GetLevel3Platforms() const { return m_level3Platforms; }
	const std::vector<PlatformButton>& GetLevel3Buttons() const { return m_level3Buttons; }
	const std::vector<PlatformObstacle>& GetLevel3Obstacles() const { return m_level3Obstacles; }

    const std::vector<Platform>& GetBossPlatforms()  const { return m_bossPlatforms; }
    
    const std::vector<Platform>& GetWallPlatforms()  const { return m_wallPlatforms; }
	const std::vector<Platform>& GetLevel3WallPlatforms() const { return m_level3WallPlatforms; }
     
    const std::vector<Checkpoint>& GetCheckpoints() const { return m_checkpoints; }

    // Non?const accessors for vectors that may be modified (add 3 and 4 here when needed)
    std::vector<Platform>& GetLevel2Platforms() { return m_level2Platforms; }
    std::vector<PlatformButton>& GetLevel2Buttons() { return m_level2Buttons; }

    // Current background section (0 based)
    int GetCurrentSection() const { return m_currentSection; }

    // Accessor for HUD & UI elements
    HUD & GetHUD() { return m_HUD; }
    const HUD& GetHUD() const { return m_HUD; }

    // Checkpoint handling
    bool HandleCheckpoint(bool checkpointHit);
    void RequestSave();  // sets internal save request flag

    //clean up functions
    void Clear();   // clears all environment vectors

private:
    EnvironmentManager() = default;   // private constructor
    ~EnvironmentManager() = default;

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

    std::vector<Platform>         m_level1Platforms;
    std::vector<Platform>         m_level2Platforms;
    std::vector<Platform>         m_level3Platforms;
    std::vector<Platform>         m_bossPlatforms;
    std::vector<Platform>         m_wallPlatforms;
    std::vector<Platform>         m_level3WallPlatforms;
    std::vector<PlatformObstacle> m_level1Obstacles;
    std::vector<PlatformObstacle> m_level2Obstacles;
	std::vector<PlatformObstacle> m_level3Obstacles;
    std::vector<Checkpoint>       m_checkpoints;
    std::vector<PlatformButton>   m_level1Buttons;
    std::vector<PlatformButton>   m_level2Buttons;
	std::vector<PlatformButton>   m_level3Buttons;

    bool m_checkpointSaved = false;       // replaces static local in MainGame
    bool m_saveRequested = false;         // internal save request flag

    int m_previousSelection = -1;   // for level indicator
    int m_currentSection = 0;

    // Background constants
    static constexpr int BACKGROUND_SECTIONS = 4;
    float m_sectionHeights[BACKGROUND_SECTIONS] = { 1900.0f, 4550.0f, 9600.0f, 10600.0f };

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
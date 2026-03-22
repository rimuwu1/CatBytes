/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.h
\author     Joash ng, joash.ng, 2502780
            Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
            Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
            Sim Hui Min, s.huimin, 2503506
\par        joash.ng@digipen.edu
            kerwinjiajie.wong@digipen.edu
            p.yuxuanlovette@digipen.edu
            s.huimin@digipen.edu
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
#include "SpatialGrid.h"
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
    void LoadAssetsFromConfig(const rapidjson::Document& doc);
    void Initialize();
    void Update(float dt, Player& player, float cameraY);
    void DrawBackground() const;
    void DrawBackgroundOverlay(float camX, float camY) const;
    void DrawWorld(float camX, float camY, PlayerWeapon weapon, const Player& player, float screenHalfH);
    void DrawHUD(float camX, float camY, PlayerWeapon weapon);

    const std::vector<PlatformObstacle>& GetCurrentObstacles() const;

    // Accessors for collision and gameplay systems
    const std::vector<Platform>& GetLevel1Platforms() const { return m_level1Platforms; }
    const std::vector<PlatformButton>& GetLevel1Buttons() const { return m_level1Buttons; }
    const std::vector<PlatformObstacle>& GetLevel1Obstacles() const { return m_level1Obstacles; }

    const std::vector<Platform>& GetLevel2Platforms() const { return m_level2Platforms; }
    const std::vector<PlatformButton>& GetLevel2Buttons() const { return m_level2Buttons; }
    const std::vector<PlatformObstacle>& GetLevel2Obstacles() const { return m_level2Obstacles; }
    const std::vector<PlatformLaser>& GetLevel2Lasers() const { return m_level2Lasers; }

    const std::vector<Platform>& GetLevel3Platforms() const { return m_level3Platforms; }
    const std::vector<PlatformButton>& GetLevel3Buttons() const { return m_level3Buttons; }
    const std::vector<PlatformObstacle>& GetLevel3Obstacles() const { return m_level3Obstacles; }
    const std::vector<PlatformLaser>& GetLevel3Lasers() const { return m_level3Lasers; }
    const std::vector<PlatformComputer>& GetLevel3Computers() const { return m_level3Computers; }
    const std::vector<Platform>& GetLevel3ToggleWalls() const { return m_level3ToggleWalls; }

    const std::vector<Platform>& GetBossPlatforms()  const { return m_bossPlatforms; }
    
    const std::vector<Platform>& GetWallPlatforms()  const { return m_wallPlatforms; }
    const std::vector<Platform>& GetLevel3WallPlatforms() const { return m_level3WallPlatforms; }
     
    const std::vector<Checkpoint>& GetCheckpoints() const { return m_checkpoints; }

    // Non-const accessors for vectors that may be modified (add 3 and 4 here when needed)
    std::vector<Platform>& GetLevel2Platforms() { return m_level2Platforms; }
    std::vector<PlatformButton>& GetLevel2Buttons() { return m_level2Buttons; }

    // Current background section (0 based)
    int GetCurrentSection() const { return m_currentSection; }
    float GetSectionHeight(int index) const { return m_sectionHeights[index]; }

    // Spatial partitioning
    SpatialGrid& GetSpatialGrid() { return m_spatialGrid; }
    const SpatialGrid& GetSpatialGrid() const { return m_spatialGrid; }

    // Accessor for HUD & UI elements
    HUD & GetHUD() { return m_HUD; }
    const HUD& GetHUD() const { return m_HUD; }

    // Checkpoint handling
    bool HandleCheckpoint(bool checkpointHit); //not used anymore
    bool isSaveRequested(); //saves once then clears state
    void RequestSave();  // sets internal save request flag
    void SetCheckpointInRange(bool inRange);
    bool GetCheckpointInRange() const;

    //clean up functions
    void Clear();   // clears all environment vectors

    // Static batch cache invalidation
    void MarkStaticDirty();

    //button clip struct
    struct ButtonClipConfig {
        std::string name;
        int start = 0;
        int end = 0;
        float duration = 0.0f;
        bool loop = false;
    };
    
    // boss door
    struct BossDoor {
        float x = 0, y = 0, w = 30.0f, h = 100.0f;
        float liftX = 0, liftY = 0;
        float liftW = 60.0f, liftH = 200.0f;
        float triggerRadius = 150.0f;
        std::string prompt;
        AEGfxTexture* doorTex = nullptr;
        std::unique_ptr<SpriteSheet> liftAnim;
        bool playerNear  = false;
        bool activated   = false;
    };

    struct BossLiftSequence {
        bool  active     = false;
        float camY       = 0.0f;
        float liftPosY   = 0.0f;
        float fadeAlpha  = 0.0f;
        bool  fadingIn   = false;
        static constexpr float LIFT_SPEED = 400.0f;
        static constexpr float FADE_START = 9100.0f;
        static constexpr float FADE_SPEED = 1.5f;
        static constexpr float LIFT_END   = 11000.0f;
        static constexpr float CAM_START_Y = 7640.0f;
    };

    // getters  
    BossDoor& GetBossDoor() { return m_bossDoor; }
    BossLiftSequence& GetLiftSequence() { return m_liftSeq; }
    bool               IsBossDoorLoaded() const { return m_bossDoorLoaded; }
    bool               IsLiftActive()     const { return m_liftSeq.active; }

   
    void SetBossRoomMode(bool enabled) { m_bossRoomMode = enabled; }
    bool IsBossRoomMode() const { return m_bossRoomMode; }

    // boss room
    void LoadBossArenaFromConfig(const rapidjson::Document& doc);

    // Parallax layer data structure
    struct ParallaxLayer {
        AEGfxTexture* texture = nullptr;
        float speed = 1.0f;  // parallax speed multiplier
    };

private:
    EnvironmentManager() = default;   // private constructor
    ~EnvironmentManager() = default;

    // Background system (embedded from Background.cpp)
    void UpdateBackground(float cameraY);
    int  GetSectionFromY(float y) const;

    // Level indicator (using existing free functions)
    void UpdateLevelIndicator(float dt);
    void DrawLevelIndicator() const;

    // Static batch cache helpers
    void RebuildStaticCache();
    void FlushStaticCache(float camY, float cullHalf);

    // Platform textures
    std::unique_ptr<SpriteSheet> m_hoverAnim;
    std::unique_ptr<SpriteSheet> m_checkpointAnim;
    std::unique_ptr<SpriteSheet> m_spikeAnim;
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
    std::vector<Platform>         m_level3ToggleWalls;
    std::vector<PlatformObstacle> m_level1Obstacles;
    std::vector<PlatformObstacle> m_level2Obstacles;
    std::vector<PlatformObstacle> m_level3Obstacles;
    std::vector<Checkpoint>       m_checkpoints;
    std::vector<PlatformButton>   m_level1Buttons;
    std::vector<PlatformButton>   m_level2Buttons;
    std::vector<PlatformButton>   m_level3Buttons;
    std::vector<PlatformLaser>    m_level2Lasers;
    std::vector<PlatformLaser>    m_level3Lasers;
    std::vector<PlatformComputer> m_level3Computers;

    bool m_checkpointSaved = false;       // replaces static local in MainGame
    bool m_saveRequested = false;         // internal save request flag
    bool m_checkpointInRange = false;     // true if player is near a checkpoint

    //button stuff
    std::string m_buttonFilePath = "Assets/Images/buttonSheet2.png";
    int m_buttonRows = 3;
    int m_buttonCols = 4;
    int m_buttonTotalFrames = 9;
    float m_buttonFrameDuration = 0.1f;
    std::vector<ButtonClipConfig> m_buttonClips;

    // computer (also uses button clip config)
    std::string m_computerFilePath = "Assets/Images/computer.png";
    int m_computerRows = 2;
    int m_computerCols = 4;
    int m_computerTotalFrames = 8;
    float m_computerFrameDuration = 0.1f;
    std::vector<ButtonClipConfig> m_computerClips;

    // laser indicator config (horizontal)
    std::string m_leftIndicatorFilePath = "Assets/Images/laserIndicatorLeft.png";
    std::string m_rightIndicatorFilePath = "Assets/Images/laserIndicatorRight.png";
    int m_hIndicatorRows = 1;
    int m_hIndicatorCols = 12;
    int m_hIndicatorTotalFrames = 12;
    float m_hIndicatorFrameDuration = 0.1f;
    std::vector<ButtonClipConfig> m_hIndicatorClips;

    // laser indicator config (vertical)
    std::string m_verticalIndicatorFilePath = "Assets/Images/laserIndicatorVerticle.png";
    int m_vIndicatorRows = 1;
    int m_vIndicatorCols = 12;
    int m_vIndicatorTotalFrames = 12;
    float m_vIndicatorFrameDuration = 0.1f;
    std::vector<ButtonClipConfig> m_vIndicatorClips;

    // laser texture
    AEGfxTexture* m_laserTex = nullptr;

    // wall texture
    AEGfxTexture* m_wallLeftTex = nullptr;
    AEGfxTexture* m_wallMidTex = nullptr;
    AEGfxTexture* m_wallRightTex = nullptr;

    int m_previousSelection = -1;   // for level indicator
    int m_currentSection = 0;

    // Queued sprite for batched drawing (reused across frames)
    struct QueuedSprite {
        AEGfxTexture* texture;
        float uvW, uvH;
        float x, y, w, h;
        float uvOffX, uvOffY;
        float opacity;
        float rotation;
    };
    std::vector<QueuedSprite> m_spriteBatch;

    // Static geometry cache
    std::vector<QueuedSprite> m_staticCache;
    bool m_staticBatchDirty = true;

    // Spatial partitioning grid
    SpatialGrid m_spatialGrid;

    // Parallax layers (back, middle, front)
    ParallaxLayer m_parallaxLayers[3];
    float m_parallaxY = 0.0f;  // track current parallax offset

    // Background constants
    static constexpr int BACKGROUND_SECTIONS = 4;
    float m_sectionHeights[BACKGROUND_SECTIONS] = { 1900.0f, 4550.0f, 9600.0f, 10600.0f };

    struct Colour { float r, g, b, a; };
    Colour m_backgroundColours[BACKGROUND_SECTIONS] = {
        { 221 / 255.0f, 160 / 255.0f, 221 / 255.0f, 1.0f},   // level 1 plum
        { 186 / 255.0f, 85 / 255.0f, 211 / 255.0f, 1.0f},   // level 2 violet
        {104 / 255.0f,  26 / 255.0f, 163 / 255.0f, 1.0f},   // level 3 purple
        { 43 / 255.0f,   4 / 255.0f,  56 / 255.0f, 1.0f}    // level 4 dark purple
    };
    Colour m_currentColour = m_backgroundColours[0];

    static Colour BlendColours(const Colour& a, const Colour& b, float t);

    // boss door members
    BossDoor         m_bossDoor{};
    BossLiftSequence m_liftSeq{};
    bool             m_bossDoorLoaded = false;

    bool m_bossRoomMode = false;
};

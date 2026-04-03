/* Start Header ************************************************************************/
/*!
\file MainGame.cpp
\author Joash ng, joash.ng, 2502780
        Sim Hui Min, s.huimin, 2503506
        Tse Xuan Qi Tristin, tse.x, 2503757
        Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par joash.ng@digipen.edu
     s.huimin@digipen.edu
     tse.x@digipen.edu
     p.yuxuanlovette@digipen.edu
     kerwinjiajie.wong@digipen.edu
\date 21/01/2026
\brief This file implements the functions for the main gamestate of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "MainGame.h"
#include <fstream>
#include <thread>
#include "pch.h"
#include "Input.h"
#include "GameStateManager.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include "ObjectManager.h"
#include "CollisionManager.h"
#include "EnvironmentManager.h"
#include "Camera.h"
#include "WinLose.h"
#include "GameSaveManager.h"
#include "LevelIndicator.h"
#include "Player.h"
#include "Enemy.h"
#include "EnemyBullet.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "AudioManager.h"
#include "UIManager.h"
#include "DebugManager.h"
#include "ParticleManager.h"
#include "BossRoom.h"

AEAudio g_GameMusic{};
bool g_GameMusicPlaying = false;
extern AEAudio g_MainMenuMusic;
static bool g_liftStartedSoundPlayed = false;
static bool g_liftLoopPlaying = false;
static std::unordered_map<const PlatformObstacle*, bool> g_prevSpikeStates;
static std::unordered_map<const PlatformLaser*, bool> g_prevLaserStates;
static float g_damageSoundCooldown = 0.0f;

// Camera pan sequence state for button toggles
static bool  g_pendingToggle      = false;
static float g_pendingToggleTimer = 0.0f;
static float g_pendingToggleMidpt = 0.0f;
static float g_pendingToggleBtnX  = 0.0f;  // button/computer X for matching
static float g_pendingToggleBtnY  = 0.0f;  // button/computer Y for matching
static bool g_bossDoorHackPendingReturn = false;  // true when BossDoor hack done, waiting for camera to return
static CollisionManager::ToggleType g_pendingToggleType = CollisionManager::ToggleType::None;

namespace {
    rapidjson::Document configDoc;   // static to this file

    // ------------------------------------------------------------------------
    // Reads GameSave.json if it exists, otherwise falls back to GameConfig.json.
    // Clears configDoc first to prevent stale cached values bleeding through.
    // ------------------------------------------------------------------------
    static void ParseConfigFromDisk()
    {
        // Wait for any in-flight async save to finish before reading.
        // Replaced busy-wait with condition-based wait.
        GameSaveManager::WaitForSaveToFinish();

        // Clear stale data so the previous parse never bleeds into a fresh load
        configDoc.SetObject();

        std::ifstream ifs("Assets/Data/GameSave.json");
        if (!ifs.is_open()) {
            ifs.clear();
            ifs.open("Assets/Data/GameConfig.json");
        }
        rapidjson::IStreamWrapper isw(ifs);
        configDoc.ParseStream(isw);

        // Fallback if the file was corrupt or mid-write
        if (configDoc.HasParseError() || !configDoc.IsObject()) {
            configDoc.SetObject();
            std::ifstream fallback("Assets/Data/GameConfig.json");
            rapidjson::IStreamWrapper isw2(fallback);
            configDoc.ParseStream(isw2);
        }
    }

    const rapidjson::Document& GetConfigDoc() {
        return configDoc;
    }
} // anon namespace

  // Walking emitter handle
static EmitterHandle g_walkEmitter = INVALID_EMITTER;

static bool SpikeIsInCamera(float x, float y, float w, float h)
{
    const float halfScreenW = 1600.0f * 0.5f;
    const float halfScreenH = 900.0f * 0.5f;

    const float camLeft = globalCam.x - halfScreenW;
    const float camRight = globalCam.x + halfScreenW;
    const float camBottom = globalCam.y - halfScreenH;
    const float camTop = globalCam.y + halfScreenH;

    const float objLeft = x - w * 0.5f;
    const float objRight = x + w * 0.5f;
    const float objBottom = y - h * 0.5f;
    const float objTop = y + h * 0.5f;

    const bool overlapX = (objRight >= camLeft) && (objLeft <= camRight);
    const bool overlapY = (objTop >= camBottom) && (objBottom <= camTop);

    return overlapX && overlapY;
}

// ------------------------------------------------------------------------
// Applies the already-parsed GetConfigDoc() to all managers + resets camera
// ------------------------------------------------------------------------
static void ApplyConfigToManagers()
{
    EnvironmentManager::Get().LoadFromConfig(GetConfigDoc());
    EnvironmentManager::Get().LoadAssetsFromConfig(GetConfigDoc());
    ObjectManager::Get().LoadFromConfig(GetConfigDoc());

    // Clear HUD inventory before syncing from save to prevent old buffs from persisting
    HUD& hud = EnvironmentManager::Get().GetHUD();
    hud.ClearInventory();

    // Sync HUD inventory from restored player buffs
    Player& player = ObjectManager::Get().GetPlayer();
    for (const auto& buff : player.buffs) {
        if (buff.active)
            hud.AddBuffToInventory(buff.type);
    }

    const float ground = -350.0f;
    const float groundHeight = 50.0f;
    const float halfScreenHeight = 900.0f * 0.5f;
    float groundTop = ground + groundHeight * 0.5f;
    Camera_Init(globalCam, ObjectManager::Get().GetPlayer().pos.x, groundTop + halfScreenHeight);

    // Reset camera shake state when (re)starting a run
    camTrauma = 0.0f;
    camShakeTime = 0.0f;
}

// ------------------------------------------------------------------------

void MainGame_Load()
{
    // One-time setup only: textures, level indicator. NOT the JSON parse.
    // Load is skipped by the GSM on re-entry from another state,
    // so anything that must run every time belongs in Initialize instead.
    EnvironmentManager::Get().Initialize();
    std::cout << "MainGame:Load" << std::endl;
}

void MainGame_Initialize()
{
    // Clear any leftover popup state from a previous visit to this screen
    UIManager::Get().Reset();
    g_prevSpikeStates.clear();

    //reset flags so they play
    g_liftStartedSoundPlayed = false;
    g_liftLoopPlaying = false;

    // Reset camera sequence state
    g_pendingToggle      = false;
    g_pendingToggleTimer = 0.0f;
    g_pendingToggleType  = CollisionManager::ToggleType::None;
    g_camSequenceActive  = false;
    g_bossDoorHackPendingReturn = false;

    // clear old game objects/environment before reloading from JSON
    ObjectManager::Get().Clear();
    EnvironmentManager::Get().Clear();

    // If data wasn't loaded during splash screen, parse JSON now
    // The managers need to be reloaded every time because we Clear() above
    if (!IsGameDataLoaded()) {
        ParseConfigFromDisk();
        AudioManager::Get().LoadFromJson(GetConfigDoc()["audio"]);
        DebugManager::Get().Initialize();
        ParticleManager_Init();
    }
    
    // Always reload managers from the parsed config (handles Clear() above)
    // Note: This is fast since textures are already cached by TextureManager
    EnvironmentManager::Get().LoadFromConfig(GetConfigDoc());
    EnvironmentManager::Get().LoadAssetsFromConfig(GetConfigDoc());
    ObjectManager::Get().LoadFromConfig(GetConfigDoc());
    
    // Sync HUD inventory from restored player buffs
    HUD& hud = EnvironmentManager::Get().GetHUD();
    hud.ClearInventory();
    Player& player = ObjectManager::Get().GetPlayer();
    for (const auto& buff : player.buffs) {
        if (buff.active)
            hud.AddBuffToInventory(buff.type);
    }
    
    // Initialize camera
    const float ground = -350.0f;
    const float groundHeight = 50.0f;
    const float halfScreenHeight = 900.0f * 0.5f;
    float groundTop = ground + groundHeight * 0.5f;
    Camera_Init(globalCam, player.pos.x, groundTop + halfScreenHeight);
    camTrauma = 0.0f;
    camShakeTime = 0.0f;
    
    g_walkEmitter = INVALID_EMITTER;
    //Stop main menu music
    AudioManager::Get().StopAudio(g_MainMenuMusic);
    //Start game music (looped)
    g_GameMusic = AudioManager::Get().GetAudio("game_music");
    AudioManager::Get().PlayAudio(g_GameMusic, true); // loop = true
    AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("game_start"), false);
    g_GameMusicPlaying = true;
    std::cout << "MainGame:Initialize" << std::endl;
}

void MainGame_Update()
{
    //Let UIManager process any active popup first
    if (UIManager::Get().Update(globalCam.x, globalCam.y)) {
        return;
    }

    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (g_damageSoundCooldown > 0.0f) g_damageSoundCooldown -= dt;
    if (DebugManager::Get().Update(dt)) return;
    if (AEInputCheckTriggered(AEVK_ESCAPE)) { //moved from input cos update pause is tweaking
        UIManager::Get().ShowPause(globalCam.x, globalCam.y);
    }

    Player& player = ObjectManager::Get().GetPlayer();
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    //auto& enemyBullets = ObjectManager::Get().GetAllEnemyBullets();

    // Restart handling
    if (g_newGame) {
        MainGame_Initialize();
        g_newGame = false;
        return;
    }

    float playerPrevY = player.pos.y;

    // ----- Boss door auto-save (happens even during lift sequence) -----
    if (EnvironmentManager::Get().isSaveRequested())
    {
        // If boss door was activated, restore original position for save (player is hidden off-screen)
        bool bossDoorTriggered = EnvironmentManager::Get().IsBossDoorLoaded() && 
            EnvironmentManager::Get().GetBossDoor().activated;
        float originalX = player.pos.x;
        float originalY = player.pos.y;

        if (bossDoorTriggered) {
            player.pos.x = EnvironmentManager::Get().GetBossDoor().savedPlayerX;
            player.pos.y = EnvironmentManager::Get().GetBossDoor().savedPlayerY;
        }

        int currentSection = EnvironmentManager::Get().GetCurrentSection();
        int currentLevel = currentSection + 1;
        const std::vector<Platform>* currentPlatforms = nullptr;
        switch (currentLevel) {
        case 1:  currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
        case 2:  currentPlatforms = &EnvironmentManager::Get().GetLevel2Platforms(); break;
        case 3:  currentPlatforms = &EnvironmentManager::Get().GetLevel3Platforms(); break;
        case 4:  currentPlatforms = &EnvironmentManager::Get().GetBossPlatforms();   break;
        default: currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
        }

        GameSaveManager::Metadata meta{ "", currentLevel, currentSection, static_cast<int>(ObjectManager::Get().GetPlayerHP()) };

        float levelMinY = (currentSection == 0) ? -FLT_MAX : EnvironmentManager::Get().GetSectionHeight(currentSection - 1);
        float levelMaxY = EnvironmentManager::Get().GetSectionHeight(currentSection);

        const std::vector<Buff>& worldBuffs = ObjectManager::Get().GetAllBuffs();
        const auto& toggleWalls = EnvironmentManager::Get().GetLevel3ToggleWalls();
        GameSaveManager::SaveGameAsync(meta, currentLevel, player, enemies, *currentPlatforms, worldBuffs, toggleWalls, levelMinY, levelMaxY);
        GameSaveManager::WaitForSaveToFinish();
        GameSaveManager::Notify_Show(GameSaveManager::NotifyType::SAVED);
        AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("save"), false);
        ParticleManager_Emit(player.pos.x, player.pos.y, 15, 200.f, 255, 255, 255);
        EnvironmentManager::Get().PatchBossDoorLockedInSave();

        // Restore hidden position after save
        if (bossDoorTriggered) {
            player.pos.x = originalX;
            player.pos.y = originalY;
        }
    }

    // ================== GAMEPLAY LOGIC (paused during camera sequence) ==================
    if (!g_camSequenceActive && !EnvironmentManager::Get().IsLiftActive()) {
        for (const auto& e : enemies) {
            if (e.justDied) {
                Camera_AddTrauma(0.3f);
                ParticleManager_Emit(e.pos.x, e.pos.y, 20, 300.f, 191, 64, 255); //purple cos them enemies are purple
            }
        }

        ObjectManager::Get().Update(dt);

        // Update the BossRoom singleton (handles boss laser collisions and logic)
        BossRoom::Get().Update(dt);

        // Walking dust emitter — start when grounded and moving, stop otherwise
        bool isWalking = player.grounded && fabs(player.vel.x) > 10.0f;
        if (isWalking) {
            if (g_walkEmitter == INVALID_EMITTER) {
                // Emit from player feet, grey/white dust, slow upward drift
                g_walkEmitter = ParticleManager_EmitterStart(
                    player.pos.x, player.pos.y - player.height * 0.5f,
                    12, 60.f, 200, 200, 200, 0.15f, 0.3f, 3.0f, 6.0f);
            } else {
                ParticleManager_EmitterMove(g_walkEmitter,
                    player.pos.x, player.pos.y - player.height * 0.5f);
            }
        } else {
            if (g_walkEmitter != INVALID_EMITTER) {
                ParticleManager_EmitterStop(g_walkEmitter);
                g_walkEmitter = INVALID_EMITTER;
            }
        }

        ParticleManager_Update(dt);

        // Noclip: override gravity and allow free vertical movement
        if (DebugManager::Get().IsNoclipActive()) {
            player.vel.y = 0.0f;
            player.grounded = 1;
            if (AEInputCheckCurr(AEVK_W)) player.pos.y += 400.f * dt;
            if (AEInputCheckCurr(AEVK_S)) player.pos.y -= 400.f * dt;
        }

        ObjectManager::Get().RebuildSpatialGrid();

        // ================== COLLISION HANDLING ==================
        auto results = CollisionManager::HandleAllCollisionsSpatial(
            player,
            playerPrevY,
            EnvironmentManager::Get(),
            ObjectManager::Get().GetAllEnemies()
        );

        //obstacle reaction
        if (results.obstacleHit)
        {
            const bool canTakeObstacleDamage = (player.hurtTimer <= 0.0f);

            if (canTakeObstacleDamage)
            {
                Player_ApplyDamage(player, 1.0f);

                if (g_damageSoundCooldown <= 0.0f)
                {
                    AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("damage"), false);
                    g_damageSoundCooldown = 0.25f;
                }

                // Knockback horizontally away from player's movement direction
                float knockDir = player.facingRight ? -1.0f : 1.0f;
                if (player.vel.x > 0.0f)      knockDir = -1.0f;
                else if (player.vel.x < 0.0f) knockDir =  1.0f;

                player.knockbackVel.x = knockDir * player.knockbackVelocity;
                player.knockbackVel.y = player.grounded ? 0.0f : player.knockbackAirUp;
                player.knockbackTimer = player.hurtTimer;
                player.vel.x = player.knockbackVel.x;
                player.vel.y = player.knockbackVel.y;
            }
        }

        // Pogo reaction
        if (results.pogoHit) {
            if (!player.pogoJustPerformed) {
                player.vel.y = player.pogoVelocity;
                player.pogoJustPerformed = true;
                player.grounded = false;
                player.downSlashJumped = false;
            }
        }
        else {
            player.pogoJustPerformed = false;
        }

        // ----- Camera pan sequence for button toggles -----
        // Start camera sequence if a button was just triggered
        if (results.pendingToggle.triggered && !g_camSequenceActive && !g_pendingToggle) {

            Camera_StartSequence(results.pendingToggle.targetY, globalCam.y);
            g_pendingToggle      = true;
            g_pendingToggleMidpt = g_camSequenceDuration + g_camHoldDuration * 0.5f;
            g_pendingToggleTimer = 0.0f;
            g_pendingToggleBtnX  = results.pendingToggle.buttonX;
            g_pendingToggleBtnY  = results.pendingToggle.buttonY;
            g_pendingToggleType  = results.pendingToggle.type;
        }

        // ----- Camera pan sequence for computer toggles (lasers) -----
        if (results.pendingCameraPan) {
            AudioManager::Get().PlayAudio("computer", false);
            float indicatorTime = 1.5f;
            float beamVisible = 1.0f;
            float camHoldTime = indicatorTime + beamVisible;
            Camera_StartSequence(results.cameraPanTargetY, globalCam.y, 0.6f, camHoldTime);
            AudioManager::Get().PlayAudio("laser_on", false);
        }

        // ----- camera pan down to boss door when the pc unlocks it -----
        if (results.pendingComputer.triggered &&
            results.pendingComputer.type == CollisionManager::ToggleType::BossDoorUnlock &&
            !g_camSequenceActive && !g_pendingToggle)
        {
            AudioManager::Get().PlayAudio("computer", false);
            // pan down to door; unlock fires at midpoint so the player sees it change state
            Camera_StartSequence(results.pendingComputer.targetY, globalCam.y, 0.6f, 2.0f);
            AudioManager::Get().PlayAudio("elevator_unlocked", false);
            g_pendingToggle      = true;
            g_pendingToggleMidpt = g_camSequenceDuration + g_camHoldDuration * 0.5f;
            g_pendingToggleTimer = 0.0f;
            g_pendingToggleBtnX  = results.pendingComputer.buttonX;
            g_pendingToggleBtnY  = results.pendingComputer.buttonY;
            g_pendingToggleType  = CollisionManager::ToggleType::BossDoorUnlock;
            g_bossDoorHackPendingReturn = true;
        }

        // Lock player movement during camera sequence
        if (g_camSequenceActive) {
            player.vel.x = 0.0f;
            // suppress jump input by zeroing velocity but keep grounded state
            player.vel.y = (player.grounded ? 0.0f : player.vel.y);
        }

        // ----- Checkpoint save (only manual checkpoint trigger, not boss door) -----
        EnvironmentManager::Get().SetCheckpointInRange(results.checkpointInRange);

        if (results.checkpointInRange && AEInputCheckTriggered('E'))
        {
            int currentSection = EnvironmentManager::Get().GetCurrentSection();
            int currentLevel = currentSection + 1;
            const std::vector<Platform>* currentPlatforms = nullptr;
            switch (currentLevel) {
            case 1:  currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
            case 2:  currentPlatforms = &EnvironmentManager::Get().GetLevel2Platforms(); break;
            case 3:  currentPlatforms = &EnvironmentManager::Get().GetLevel3Platforms(); break;
            case 4:  currentPlatforms = &EnvironmentManager::Get().GetBossPlatforms();   break;
            default: currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
            }

            GameSaveManager::Metadata meta{ "", currentLevel, currentSection, static_cast<int>(ObjectManager::Get().GetPlayerHP()) };

            float levelMinY = (currentSection == 0) ? -FLT_MAX : EnvironmentManager::Get().GetSectionHeight(currentSection - 1);
            float levelMaxY = EnvironmentManager::Get().GetSectionHeight(currentSection);

            const std::vector<Buff>& worldBuffs = ObjectManager::Get().GetAllBuffs();
            const auto& toggleWalls = EnvironmentManager::Get().GetLevel3ToggleWalls();
            GameSaveManager::SaveGameAsync(meta, currentLevel, player, enemies, *currentPlatforms, worldBuffs, toggleWalls, levelMinY, levelMaxY);
            GameSaveManager::WaitForSaveToFinish();
            GameSaveManager::Notify_Show(GameSaveManager::NotifyType::SAVED);
            AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("save"), false);
            ParticleManager_Emit(player.pos.x, player.pos.y, 15, 200.f, 255, 255, 255);
            EnvironmentManager::Get().PatchBossDoorLockedInSave();
        }

        if (ObjectManager::Get().IsBossDefeated()) {
            textScreenMessage = "You Win";
            GameStateManager::Get().next = GS_WINLOSE;
        }
    }
    // ================== END GAMEPLAY LOGIC ==================

    // Fire the toggle at the midpoint of the sequence (during hold phase)
    if (g_pendingToggle) {
        g_pendingToggleTimer += dt;
        if (g_pendingToggleTimer >= g_pendingToggleMidpt) {
            // Fire the toggle based on type
            switch (g_pendingToggleType) {
            case CollisionManager::ToggleType::Platform: {
                // Find and fire the matching button for platform toggles
                auto firePlatformButton = [&](const std::vector<PlatformButton>& buttons,
                    std::vector<Platform>& platforms) {
                        for (const auto& btn : buttons) {
                            if (fabs(btn.x - g_pendingToggleBtnX) < 1.0f &&
                                fabs(btn.y - g_pendingToggleBtnY) < 1.0f) {
                                for (int idx : btn.platformIndices) {
                                    if (idx >= 0 && idx < (int)platforms.size())
                                    {
                                        platforms[idx].active = !platforms[idx].active;
                                        AudioManager::Get().PlayAudio(
                                            AudioManager::Get().GetAudio(
                                                platforms[idx].active ? "platform_appear" : "platform_disappear"
                                            ),
                                            false
                                        );
                                    }
                                }
                                EnvironmentManager::Get().MarkStaticDirty();
                                return true;
                            }
                        }
                        return false;
                    };

                const auto& level1Buttons = EnvironmentManager::Get().GetLevel1Buttons();
                const auto& level2Buttons = EnvironmentManager::Get().GetLevel2Buttons();
                const auto& level3Buttons = EnvironmentManager::Get().GetLevel3Buttons();

                if (!firePlatformButton(level1Buttons, const_cast<std::vector<Platform>&>(EnvironmentManager::Get().GetLevel1Platforms())))
                    if (!firePlatformButton(level2Buttons, const_cast<std::vector<Platform>&>(EnvironmentManager::Get().GetLevel2Platforms())))
                        firePlatformButton(level3Buttons, const_cast<std::vector<Platform>&>(EnvironmentManager::Get().GetLevel3Platforms()));
                break;
            }
            case CollisionManager::ToggleType::Wall: {
                // Find and fire the matching button for wall toggles (level 3 only)
                const auto& level3Buttons = EnvironmentManager::Get().GetLevel3Buttons();
                auto& toggleWalls = const_cast<std::vector<Platform>&>(EnvironmentManager::Get().GetLevel3ToggleWalls());

                for (const auto& btn : level3Buttons) {
                    if (fabs(btn.x - g_pendingToggleBtnX) < 1.0f &&
                        fabs(btn.y - g_pendingToggleBtnY) < 1.0f) {
                        for (int idx : btn.wallIndices) {
                            if (idx >= 0 && idx < (int)toggleWalls.size())
                            {
                                toggleWalls[idx].active = !toggleWalls[idx].active;
                                AudioManager::Get().PlayAudio(
                                    AudioManager::Get().GetAudio(
                                        toggleWalls[idx].active ? "platform_appear" : "platform_disappear"
                                    ),
                                    false
                                );
                            }
                        }
                        EnvironmentManager::Get().MarkStaticDirty();
                        break;
                    }
                }
                break;
            }
            case CollisionManager::ToggleType::Laser: {
                // Find and fire the matching computer for laser toggles (level 3 only)
                const auto& level3Computers = EnvironmentManager::Get().GetLevel3Computers();
                const auto& lasers = EnvironmentManager::Get().GetLevel3Lasers();

                for (const auto& comp : level3Computers) {
                    if (fabs(comp.x - g_pendingToggleBtnX) < 1.0f &&
                        fabs(comp.y - g_pendingToggleBtnY) < 1.0f) {
                        for (int idx : comp.laserIndices) {
                            if (idx >= 0 && idx < (int)lasers.size())
                                lasers[idx].laserActive = !lasers[idx].laserActive;
                        }
                        break;
                    }
                }
                break;
            }
            case CollisionManager::ToggleType::BossDoorUnlock: {
                // fire the actual unlock now that the camera is on the door
                if (EnvironmentManager::Get().IsBossDoorLoaded())
                    EnvironmentManager::Get().GetBossDoor().locked = false;
                break;
            }
            default:
                break;
            }

            g_pendingToggle = false;
            g_pendingToggleType = CollisionManager::ToggleType::None;
        }
    }

    // Check if BossDoor hack camera pan just completed
    if (g_bossDoorHackPendingReturn && !Camera_IsSequenceActive()) {
        g_bossDoorHackPendingReturn = false;
        // Trigger success effects: prompt change + particles
        EnvironmentManager::Get().TriggerBossDoorHackSuccess();
    }

    if (!Camera_UpdateSequence(globalCam, dt)) {
        // sequence inactive — normal follow
        if (DebugManager::Get().IsDebugCameraEnabled())
            Camera_Debug(globalCam, dt);
        else
            Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
    }

    for (auto& comp : EnvironmentManager::Get().GetLevel3Computers())
    {
        if (comp.nowShowBeam)
        {
            if (!comp.beamKilled)
            {
                comp.beamKilled = true;
                Camera_AddTrauma(0.6f);

                // kill all enemies within beam's range
                float beamLeft = std::min(comp.beamStartX, comp.beamEndX) - comp.beamW * 0.5f;
                float beamRight = std::max(comp.beamStartX, comp.beamEndX) + comp.beamW * 0.5f;
                float beamBot = std::min(comp.beamStartY, comp.beamEndY);
                float beamTop = std::max(comp.beamStartY, comp.beamEndY);

                for (auto& e : enemies)
                {
                    if (!e.isAlive) continue;

                    float enemyLeft = e.pos.x - e.width * 0.5f;
                    float enemyRight = e.pos.x + e.width * 0.5f;
                    float enemyBot = e.pos.y - e.height * 0.5f;
                    float enemyTop = e.pos.y + e.height * 0.5f;

                    bool inBeamX = (enemyRight >= beamLeft) && (enemyLeft <= beamRight);
                    bool inBeamY = (enemyTop >= beamBot) && (enemyBot <= beamTop);

                    if (inBeamX && inBeamY) {
                        e.isAlive = false;
                        ParticleManager_Emit(e.pos.x, e.pos.y, 20, 300.0f, 191, 64, 255);
                    }
                }

            }

            comp.beamDuration -= dt;

            // hide beam and indicators after kill
            if (comp.beamDuration <= 0.0f)
            {
                comp.beamActive = false;
                comp.beamVisible = false;
                comp.indicatorsVisible = false;
                comp.nowShowBeam = false;
                comp.beamKilled = false;
                comp.beamDuration = 0.0f;
            }

        }

    }


    // boss lift sequence: override camera  
    if (EnvironmentManager::Get().IsLiftActive())
    {
        if (!g_liftStartedSoundPlayed)
        {
            AudioManager::Get().PlayAudio(
                AudioManager::Get().GetAudio("elevator_ding"),
                false
            );
            g_liftStartedSoundPlayed = true;
        }
        if (!g_liftLoopPlaying)
        {
            AudioManager::Get().PlayAudio(
                AudioManager::Get().GetAudio("elevator_sound"),
                true);
            g_liftLoopPlaying = true;
        }

        EnvironmentManager::BossLiftSequence& seq = EnvironmentManager::Get().GetLiftSequence();
        globalCam.x = 0.0f;
        globalCam.y = seq.camY;

        // player lock
        player.vel.x = 0.0f;
        player.vel.y = 0.0f;
        player.grounded = 1;
    }

    Camera_UpdateShake(globalCam, dt);
    Camera_Apply(globalCam);

    float backgroundY = DebugManager::Get().IsDebugCameraEnabled() ? globalCam.y : player.pos.y;
    EnvironmentManager::Get().Update(dt, player, backgroundY);

    auto checkSpikeToggle = [&](const std::vector<PlatformObstacle>& obstacles)
        {
            for (const auto& o : obstacles)
            {
                if (!o.isSpike) continue;

                auto it = g_prevSpikeStates.find(&o);

                if (it == g_prevSpikeStates.end())
                {
                    // First time seeing this spike
                    g_prevSpikeStates[&o] = o.active;
                }
                else
                {
                    // Detect toggle
                    if (it->second != o.active)
                    {
                        if (SpikeIsInCamera(o.x, o.y, o.w, o.h))
                        {
                            AudioManager::Get().PlayAudio(
                                AudioManager::Get().GetAudio("spikes_up_down"),
                                false
                            );
                        }

                        it->second = o.active;
                    }
                }
            }
        };



    auto checkLaserToggle = [&](const std::vector<PlatformLaser>& lasers)
        {
            for (const auto& l : lasers)
            {
                auto it = g_prevLaserStates.find(&l);

                if (it == g_prevLaserStates.end())
                {
                    // First time seeing this laser
                    g_prevLaserStates[&l] = l.laserActive;
                }
                else
                {
                    // Detect toggle
                    if (it->second != l.laserActive)
                    {
                        // Compute bounds(same logic style as spikes)
                        float midX = (l.x1 + l.x2) * 0.5f;
                        float midY = (l.y1 + l.y2) * 0.5f;
                        float w = fabs(l.x2 - l.x1) + l.w;
                        float h = fabs(l.y2 - l.y1) + l.w;

                        if (SpikeIsInCamera(midX, midY, w, h))
                        {
                            if (l.laserActive)
                            {
                                AudioManager::Get().PlayAudio(
                                    AudioManager::Get().GetAudio("laser_on"),
                                    false
                                );
                            }
                            else
                            {
                                AudioManager::Get().PlayAudio(
                                    AudioManager::Get().GetAudio("laser_off"),
                                    false
                                );
                            }
                        }

                        it->second = l.laserActive;
                    }
                }
            }
        };


    checkSpikeToggle(EnvironmentManager::Get().GetLevel1Obstacles());
    checkSpikeToggle(EnvironmentManager::Get().GetLevel2Obstacles());
    checkSpikeToggle(EnvironmentManager::Get().GetLevel3Obstacles());
    checkLaserToggle(EnvironmentManager::Get().GetLevel2Lasers());
    checkLaserToggle(EnvironmentManager::Get().GetLevel3Lasers());

    // Pause button -> changes gamestate to pause
    HUD& hud = EnvironmentManager::Get().GetHUD();

    if (hud.IsPauseButtonClicked(globalCam.x, globalCam.y)) {
        // Trigger press animation for pause button
        hud.TriggerPauseButtonPress();
        AudioManager::Get().PlayAudio("click_button", false);

        UIManager::Get().ShowPause(globalCam.x, globalCam.y);
        return;
    }

    GameSaveManager::Notify_Update(dt);

    std::cout << "MainGame:Update" << std::endl;
}

void MainGame_Draw()
{
    AESysFrameStart();

    Player& player = ObjectManager::Get().GetPlayer();
    EnvironmentManager::Get().DrawBackground();
    EnvironmentManager::Get().DrawBackgroundOverlay(globalCam.x, globalCam.y);
    EnvironmentManager::Get().DrawWorld(globalCam.x, globalCam.y, player.weapon, player, 900.0f * 0.5f);
    ObjectManager::Get().Draw(globalCam.x, globalCam.y, 800.0f, 450.0f);

    // Draw boss room overlays (lasers, telegraphs)
    BossRoom::Get().Draw();

    ParticleManager_Draw();
    EnvironmentManager::Get().DrawHUD(globalCam.x, globalCam.y, player.weapon);
    GameSaveManager::Notify_Draw();
    //pop up draw over everything
    DebugManager::Get().DrawWorldOverlays(globalCam.x, globalCam.y);
    DebugManager::Get().Draw(globalCam.x, globalCam.y);
    UIManager::Get().Draw(globalCam.x, globalCam.y);
    std::cout << "MainGame:Draw" << std::endl;

    AESysFrameEnd();
}

void MainGame_Free()
{
    globalCam.x = 0.0f;
    globalCam.y = 0.0f;
    AEGfxSetCamPosition(0.0f, 0.0f);
    Player_Free(ObjectManager::Get().GetPlayer());
    ObjectManager::Get().Clear();
    EnvironmentManager::Get().Clear();
    std::cout << "MainGame:Free" << std::endl;
}

void MainGame_Unload()
{
    std::cout << "MainGame:Unload" << std::endl;
}

// ============================================================================
// Splash screen loading API - runs during splash screen cutscene
// Phase 1: JSON parsing in background thread (thread-safe file I/O only)
// Phase 2: Manager loading on main thread (OpenGL texture loading)
// ============================================================================
static std::atomic<bool> g_GameDataLoaded(false);       // true when ALL loading complete
static std::atomic<bool> g_JsonParsed(false);           // true when JSON parsing done
static std::atomic<bool> g_ParsingInProgress(false);    // true while background thread running
static int g_MainThreadLoadStep = 0;                    // tracks incremental main-thread loading

void ResetGameDataLoaded()
{
    g_GameDataLoaded.store(false, std::memory_order_release);
    g_JsonParsed.store(false, std::memory_order_release);
    g_ParsingInProgress.store(false, std::memory_order_release);
    g_MainThreadLoadStep = 0;
}

bool IsGameDataLoaded()
{
    return g_GameDataLoaded.load(std::memory_order_acquire);
}

// Background thread: ONLY parses JSON files (no OpenGL calls!)
static void AsyncJsonParser()
{
    // This is the slow file I/O part - safe to run in background
    ParseConfigFromDisk();
    
    // Signal that JSON is ready for main thread to process
    g_JsonParsed.store(true, std::memory_order_release);
    g_ParsingInProgress.store(false, std::memory_order_release);
}

void StartAsyncLoading()
{
    // Don't start if already parsing, parsed, or fully loaded
    if (g_ParsingInProgress.load(std::memory_order_acquire) || 
        g_JsonParsed.load(std::memory_order_acquire) || 
        g_GameDataLoaded.load(std::memory_order_acquire)) {
        return;
    }
    
    g_ParsingInProgress.store(true, std::memory_order_release);
    g_JsonParsed.store(false, std::memory_order_release);
    g_GameDataLoaded.store(false, std::memory_order_release);
    g_MainThreadLoadStep = 0;
    
    // Spawn background thread for JSON parsing ONLY
    std::thread(AsyncJsonParser).detach();
}

// Main thread loading - call each frame from SplashScreen_Update
// Does one loading step per frame to avoid blocking the cutscene animation
// Returns true when all loading is complete
bool ContinueMainThreadLoading()
{
    // If already done, return immediately
    if (g_GameDataLoaded.load(std::memory_order_acquire)) {
        return true;
    }
    
    // Wait for JSON parsing to complete before doing manager loading
    if (!g_JsonParsed.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Incremental loading on main thread (one step per frame)
    // Each step does OpenGL texture loading which must be on main thread
    switch (g_MainThreadLoadStep) {
        case 0:
            // Step 0: Load audio (fast, no textures)
            AudioManager::Get().LoadFromJson(GetConfigDoc()["audio"]);
            g_MainThreadLoadStep++;
            break;
            
        case 1:
            // Step 1: Environment manager config parsing (fast)
            EnvironmentManager::Get().LoadFromConfig(GetConfigDoc());
            g_MainThreadLoadStep++;
            break;
            
        case 2:
            // Step 2: Environment assets (textures - must be on main thread)
            EnvironmentManager::Get().LoadAssetsFromConfig(GetConfigDoc());
            g_MainThreadLoadStep++;
            break;
            
        case 3:
            // Step 3: Object manager (may load textures)
            ObjectManager::Get().LoadFromConfig(GetConfigDoc());
            g_MainThreadLoadStep++;
            break;
            
        case 4:
            // Step 4: Debug manager & particles
            DebugManager::Get().Initialize();
            ParticleManager_Init();
            g_MainThreadLoadStep++;
            
            // All done!
            g_GameDataLoaded.store(true, std::memory_order_release);
            break;
            
        default:
            // Already completed
            break;
    }
    
    return g_GameDataLoaded.load(std::memory_order_acquire);
}

// Get loading progress (0-100) for display
int GetLoadingProgress()
{
    if (g_GameDataLoaded.load(std::memory_order_acquire)) {
        return 100;
    }
    
    // JSON parsing is ~20% of work (file I/O)
    if (!g_JsonParsed.load(std::memory_order_acquire)) {
        return g_ParsingInProgress.load(std::memory_order_acquire) ? 10 : 0;
    }
    
    // Main thread loading is steps 0-4 (5 steps), 80% of work
    // Step 0=20%, 1=36%, 2=52%, 3=68%, 4=84%, done=100%
    return 20 + (g_MainThreadLoadStep * 16);
}
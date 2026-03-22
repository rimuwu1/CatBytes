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
#include "enemy.h"
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

// Camera pan sequence state for button toggles
static bool  g_pendingToggle      = false;
static float g_pendingToggleTimer = 0.0f;
static float g_pendingToggleMidpt = 0.0f;
static float g_pendingToggleBtnX  = 0.0f;  // button/computer X for matching
static float g_pendingToggleBtnY  = 0.0f;  // button/computer Y for matching
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


// ------------------------------------------------------------------------
// Applies the already-parsed GetConfigDoc() to all managers + resets camera
// ------------------------------------------------------------------------
static void ApplyConfigToManagers()
{
    EnvironmentManager::Get().LoadFromConfig(GetConfigDoc());
    EnvironmentManager::Get().LoadAssetsFromConfig(GetConfigDoc());
    ObjectManager::Get().LoadFromConfig(GetConfigDoc());

    // Sync HUD inventory from restored player buffs
    Player& player = ObjectManager::Get().GetPlayer();
    HUD& hud = EnvironmentManager::Get().GetHUD();
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

    // Reset camera sequence state
    g_pendingToggle      = false;
    g_pendingToggleTimer = 0.0f;
    g_pendingToggleType  = CollisionManager::ToggleType::None;
    g_camSequenceActive  = false;

    // clear old game objects/environment before reloading from JSON
    ObjectManager::Get().Clear();
    EnvironmentManager::Get().Clear();

    ParseConfigFromDisk();
    AudioManager::Get().LoadFromJson(GetConfigDoc()["audio"]);
    ApplyConfigToManagers();
    DebugManager::Get().Initialize();
    ParticleManager_Init();
    g_walkEmitter = INVALID_EMITTER;
    //Stop main menu music
    AudioManager::Get().StopAudio(g_MainMenuMusic);
    //Start game music (looped)
    g_GameMusic = AudioManager::Get().GetAudio("game_music");
    AudioManager::Get().PlayAudio(g_GameMusic, true); // loop = true
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
    if (DebugManager::Get().Update(dt)) return;
    if (AEInputCheckTriggered(AEVK_ESCAPE)) { //moved from input cos update pause is tweaking
        UIManager::Get().ShowPause();
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

    // ================== GAMEPLAY LOGIC (paused during camera sequence) ==================
    if (!g_camSequenceActive) {
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
            Player_ApplyDamage(player, 1.0f);
            // Knockback horizontally away from player's movement direction
            float knockDir = player.facingRight ? -1.0f : 1.0f;
            if (player.vel.x > 0.0f) knockDir = -1.0f;
            else if (player.vel.x < 0.0f) knockDir = 1.0f;
            
            player.knockbackVel.x = knockDir * player.knockbackVelocity;
            player.knockbackVel.y = player.grounded ? 0.0f : player.knockbackAirUp;
            player.knockbackTimer = player.hurtTimer;
            player.vel.x = player.knockbackVel.x;
            player.vel.y = player.knockbackVel.y;
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
            Camera_StartSequence(results.cameraPanTargetY, globalCam.y, 0.6f, 1.5f);
       }

        // Lock player movement during camera sequence
        if (g_camSequenceActive) {
            player.vel.x = 0.0f;
            // suppress jump input by zeroing velocity but keep grounded state
            player.vel.y = (player.grounded ? 0.0f : player.vel.y);
        }

        // ----- Checkpoint & save -----
        EnvironmentManager::Get().SetCheckpointInRange(results.checkpointInRange);

        if (results.checkpointInRange && AEInputCheckTriggered('E') || EnvironmentManager::Get().isSaveRequested())
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
            
            GameSaveManager::SaveGameAsync(meta, currentLevel, player, enemies, *currentPlatforms, levelMinY, levelMaxY);
            GameSaveManager::Notify_Show(GameSaveManager::NotifyType::SAVED);
            ParticleManager_Emit(player.pos.x, player.pos.y, 15, 200.f, 255, 255, 255);
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
                                    platforms[idx].active = !platforms[idx].active;
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
                                toggleWalls[idx].active = !toggleWalls[idx].active;
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
            default:
                break;
            }

            g_pendingToggle = false;
            g_pendingToggleType = CollisionManager::ToggleType::None;
        }
    }

    if (!Camera_UpdateSequence(globalCam, dt)) {
        // sequence inactive — normal follow
        if (DebugManager::Get().IsDebugCameraEnabled())
            Camera_Debug(globalCam, dt);
        else
            Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
    }
    Camera_UpdateShake(globalCam, dt);
    Camera_Apply(globalCam);

    float backgroundY = DebugManager::Get().IsDebugCameraEnabled() ? globalCam.y : player.pos.y;
    EnvironmentManager::Get().Update(dt, player, backgroundY);

    // Pause button -> changes gamestate to pause
    HUD& hud = EnvironmentManager::Get().GetHUD();

    if (hud.IsPauseButtonClicked(globalCam.x, globalCam.y)) {
        UIManager::Get().ShowPause();
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

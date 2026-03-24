/* Start Header ************************************************************************/
/*!
\file SpriteSheet.h
\author Sim Hui Min, s.huimin, 2503506  
        Joash ng, joash.ng, 2502780
        Tse Xuan Qi Tristin, tse.x, 2503757
\par    s.huimin@digipen.edu
        joash.ng@digipen.edu
        tse.x@digipen.edu
\date 19/03/2026
\brief Implementation of Boss room with Boss enemy, obstacles, etc. Determines Win/Lose condition of the game. 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "BossRoom.h"
#include "BossAI.h"
#include "ObjectManager.h"
#include "AudioManager.h"
#include "CollisionManager.h"
#include "DebugManager.h"
#include "EnvironmentManager.h"
#include "UIManager.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "Camera.h"
#include "enemy.h"
#include "WinLose.h"
#include "LevelIndicator.h"
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

extern Camera globalCam;
extern float camTrauma;
extern float camShakeTime;

// starting cutscene when the boss room is loaded
enum class CutscenePhase {
    WalkIn,         // player walks in
    PlayerIdle,     // player stops, idles
    BossBackIdle,   // boss plays backidle for 3s
    BossTurn,       // boss plays backtofrontturn
    BossFightIdle,  // boss plays b4fightidle for 3s
    Done            // cutscene over, gameplay begins
};

static CutscenePhase g_cutscenePhase = CutscenePhase::WalkIn;
static float         g_cutsceneTimer = 0.0f;
static bool          g_cutsceneDone  = false;

static float g_bossRoomFadeAlpha = 1.0f; // start fully dark
static bool  g_bossRoomFadingIn  = true;
static float g_playerWalkTarget = -500.0f; // static gets overwritten below

static BossAIData g_bossAI;

void BossRoom_Load() {}

void BossRoom_Initialize()
{
    // bossAI initialize
    BossAI_Init(g_bossAI);
    std::cout << "BossAI:Initialize" << std::endl;

    EnvironmentManager::Get().SetBossRoomMode(true);

    g_bossRoomFadeAlpha = 1.0f;
    g_bossRoomFadingIn  = true;
    g_cutscenePhase     = CutscenePhase::WalkIn;
    g_cutsceneTimer     = 0.0f;
    g_cutsceneDone      = false;

    // clear previous state
    ObjectManager::Get().Clear();
    EnvironmentManager::Get().Clear();

    // load full config
    rapidjson::Document doc;
    std::ifstream ifs("Assets/Data/BossConfig.json"); 
    rapidjson::IStreamWrapper isw(ifs);
    doc.ParseStream(isw);
    if (doc.HasParseError() || !doc.IsObject()) return;

    // load environment and player (gives platforms + player at correct spawn)
    EnvironmentManager::Get().LoadAssetsFromConfig(doc);
    EnvironmentManager::Get().LoadBossArenaFromConfig(doc);
    ObjectManager::Get().LoadFromConfig(doc);

    if (doc.HasMember("audio"))
        AudioManager::Get().LoadFromJson(doc["audio"]);

    // load boss from "boss" key
    if (doc.HasMember("boss"))
        ObjectManager::Get().AddEnemyFromJSON(doc["boss"]);

    // HUD init
    if (doc.HasMember("ui"))
        EnvironmentManager::Get().GetHUD().InitFromConfig(doc);

    // set boss to backidle
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    for (auto& e : enemies)
        if (e.type == EnemyType::Boss && e.spriteSheet)
            e.spriteSheet->Play("backidle");

    // player starts offscreen left
    Player& player = ObjectManager::Get().GetPlayer();
    player.pos.x = -900.0f;
    player.pos.y = -270.0f;
    g_playerWalkTarget = -450.0f; // this value overwrites static variable above

    const float ground = -350.0f;
    const float groundHeight = 50.0f;
    const float halfScreenHeight = 900.0f * 0.5f;
    Camera_Init(globalCam, 0.0f, ground + groundHeight * 0.5f + halfScreenHeight);
    camTrauma = 0.0f;
    camShakeTime = 0.0f;

    if (DebugManager::Get().m_SkipToMidPhase)
    {
        DebugManager::Get().m_SkipToMidPhase = false;

        // skip cutscene
        g_cutsceneDone  = true;
        g_cutscenePhase = CutscenePhase::Done;

        // move player to arena center
        player.pos.x = -400.0f;
        player.pos.y = -300.0f;
        player.vel.x = 0.0f;
        player.vel.y = 0.0f;

        // force boss into phase transition state
        g_bossAI.phase             = BossPhase::PhaseTransition;
        g_bossAI.attackState       = BossAttackState::HurtBetweenPhase;
        g_bossAI.stateTimer        = 0.0f;
        g_bossAI.lasersEnabled     = false;
        g_bossAI.phaseTransitionDone = false;

        // set boss HP to just below 50% so phase transition is already triggered
        auto& enemies = ObjectManager::Get().GetAllEnemies();
        for (auto& e : enemies)
            if (e.type == EnemyType::Boss)
            {
                e.hitPoints = e.maxHitPoints * 0.49f;
                if (e.spriteSheet) e.spriteSheet->Play("hurtbetweenphase");
            }
    }

    UIManager::Get().Reset();
    LevelIndicator_Show(3);
    std::cout << "BossRoom:Initialize" << std::endl;
}

void BossRoom_Update()
{
    if (UIManager::Get().Update(globalCam.x, globalCam.y))
        return;

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // debug manager  
    if (DebugManager::Get().Update(dt)) return;

    if (AEInputCheckTriggered(AEVK_ESCAPE))
        UIManager::Get().ShowPause();

    // fade in from black
    if (g_bossRoomFadingIn)
    {
        g_bossRoomFadeAlpha -= dt * 1.0f;
        if (g_bossRoomFadeAlpha <= 0.0f) { g_bossRoomFadeAlpha = 0.0f; g_bossRoomFadingIn = false; }
    }

    Player& player = ObjectManager::Get().GetPlayer();
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    const float groundTop = -350.0f + 50.0f * 0.5f;  

    // ---- cutscene state machine ----
    if (!g_cutsceneDone)
    {
        // left shift skips cutscene -> not esc as it opens pause menu
        if (AEInputCheckTriggered(AEVK_LSHIFT))
        {
            g_cutsceneDone  = true;
            g_cutscenePhase = CutscenePhase::Done;
            auto& en = ObjectManager::Get().GetAllEnemies();
            for (auto& e : en)
                if (e.type == EnemyType::Boss && e.spriteSheet)
                    e.spriteSheet->Play("attackidle");
            // don't return — fall through to normal gameplay
        }
        else {
            g_cutsceneTimer += dt;

            switch (g_cutscenePhase)
            {
            case CutscenePhase::WalkIn:
            {
                player.vel.x = 300.0f;
                player.vel.y = 0.0f;
                player.grounded = 1;
                player.pos.y = groundTop + player.height * 0.5f; // pin to floor every frame
                player.facingRight = true;

                if (player.pos.x >= g_playerWalkTarget)
                {
                    player.pos.x = g_playerWalkTarget;
                    player.vel.x = 0.0f;
                    if (player.spriteSheet) player.spriteSheet->Play("idle");

                    g_cutscenePhase = CutscenePhase::PlayerIdle;
                    g_cutsceneTimer = 0.0f;
                }
                break;
            }

            case CutscenePhase::PlayerIdle:
            {
                player.vel.x = 0.0f;
                player.vel.y = 0.0f;
                player.grounded = 1;
                player.pos.y = -325.0f + player.height * 0.5f;

                if (g_cutsceneTimer >= 0.5f)
                {
                    g_cutscenePhase = CutscenePhase::BossBackIdle;
                    g_cutsceneTimer = 0.0f;
                }
                break;
            }

            case CutscenePhase::BossBackIdle:
            {
                player.vel.x = 0.0f;
                player.vel.y = 0.0f;
                player.grounded = 1;

                if (g_cutsceneTimer >= 3.0f)
                {
                    for (auto& e : enemies)
                        if (e.type == EnemyType::Boss && e.spriteSheet)
                            e.spriteSheet->Play("backtofrontturn");

                    g_cutscenePhase = CutscenePhase::BossTurn;
                    g_cutsceneTimer = 0.0f;
                }
                break;
            }

            case CutscenePhase::BossTurn:
            {
                player.vel.x = 0.0f;
                player.vel.y = 0.0f;
                player.grounded = 1;

                bool turnDone = false;
                for (auto& e : enemies)
                    if (e.type == EnemyType::Boss && e.spriteSheet)
                        turnDone = !e.spriteSheet->IsPlaying();

                if (turnDone || g_cutsceneTimer >= 1.0f)
                {
                    for (auto& e : enemies)
                        if (e.type == EnemyType::Boss && e.spriteSheet)
                            e.spriteSheet->Play("b4fightidle");

                    g_cutscenePhase = CutscenePhase::BossFightIdle;
                    g_cutsceneTimer = 0.0f;
                }
                break;
            }

            case CutscenePhase::BossFightIdle:
            {
                player.vel.x = 0.0f;
                player.vel.y = 0.0f;
                player.grounded = 1;

                if (g_cutsceneTimer >= 3.0f)
                {
                    g_cutsceneDone  = true;
                    g_cutscenePhase = CutscenePhase::Done;

                    for (auto& e : enemies)
                        if (e.type == EnemyType::Boss && e.spriteSheet)
                            e.spriteSheet->Play("attackidle");

                    // reset laser cooldown so boss doesn't fire immediately
                    for (auto& e : enemies)
                        if (e.type == EnemyType::Boss)
                            for (auto& laser : e.bossLasers)
                                laser.cooldownTimer = e.laserCooldown;
                }
                break;
            }

            default:
                g_cutsceneDone = true;
                break;
            }

            // freeze boss position during cutscene
            for (auto& e : enemies)
                if (e.type == EnemyType::Boss)
                {
                    e.vel.x = 0.0f;
                    e.vel.y = 0.0f;
                    e.state = EnemyState::Idle;
                }

            // disable all boss lasers during cutscene
            for (auto& e : enemies)
                if (e.type == EnemyType::Boss)
                    for (auto& laser : e.bossLasers)
                    {
                        laser.active = false;
                        laser.state  = BossLaserState::Inactive;
                        laser.stateTimer = 0.0f;
                        laser.cooldownTimer = e.laserCooldown; // keep resetting so it never fires
                    }

            // during cutscene: tick animations + camera, no gameplay
            ObjectManager::Get().Update(dt);
            Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
            Camera_Apply(globalCam);
            EnvironmentManager::Get().Update(dt, player, globalCam.y);
            return;

        }
    }

    // noclip
    if (DebugManager::Get().IsNoclipActive()) {
        player.vel.y = 0.0f;
        player.grounded = 1;
        if (AEInputCheckCurr(AEVK_W)) player.pos.y += 400.f * dt;
        if (AEInputCheckCurr(AEVK_S)) player.pos.y -= 400.f * dt;
    }

    ObjectManager::Get().Update(dt);
    BossRoom::Get().Update(dt);
    ObjectManager::Get().RebuildSpatialGrid();

    float playerPrevY = player.pos.y;
    CollisionManager::HandleAllCollisionsSpatial(
        player, playerPrevY,
        EnvironmentManager::Get(),
        ObjectManager::Get().GetAllEnemies()
    );

    // notify boss AI when player lands a melee/bullet hit
    static float s_prevBossHitStun = 0.0f;

    for (auto& e : enemies)
    {
        if (e.type != EnemyType::Boss) continue;
        if (g_bossAI.defeatStarted) continue;

        bool hitJustStarted = (e.hitStunTimer > 0.0f && s_prevBossHitStun <= 0.0f);
        s_prevBossHitStun = e.hitStunTimer;

        if (!hitJustStarted) continue;
        if (BossAI_IsInvincible(g_bossAI)) continue;

        //
        if (e.hitPoints <= 0.1f) continue;

        bool wasInUsePC = (g_bossAI.attackState == BossAttackState::UsePC);

        float roll = (float)rand() / (float)RAND_MAX;
        if (roll < 0.60f && !wasInUsePC)
        {
            g_bossAI.attackState = BossAttackState::Blocking;
            g_bossAI.stateTimer  = 0.0f;
        }
        else
        {
            g_bossAI.consecutiveHits++;
            g_bossAI.consecutiveHitTimer = 0.0f;

            if (wasInUsePC)
            {
                g_bossAI.usePCHitsRemaining--;
                g_bossAI.lasersEnabled       = false;
                g_bossAI.phaseTransitionDone = true; // route HurtBetweenPhase to Phase2
                g_bossAI.attackState         = BossAttackState::HurtBetweenPhase;
                g_bossAI.stateTimer          = 0.0f;
                if (e.spriteSheet)
                    e.spriteSheet->Play("hurtbetweenphase");
            }
            else
            {
                g_bossAI.attackState = BossAttackState::Hurt;
                g_bossAI.stateTimer  = 0.0f;
            }
        }
    }

    // camera — debug or follow
    if (DebugManager::Get().IsDebugCameraEnabled())
        Camera_Debug(globalCam, dt);
    else
        Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);

    Camera_UpdateShake(globalCam, dt);
    Camera_Apply(globalCam);

    float backgroundY = DebugManager::Get().IsDebugCameraEnabled() ? globalCam.y : player.pos.y;
    EnvironmentManager::Get().Update(dt, player, backgroundY);

    HUD& hud = EnvironmentManager::Get().GetHUD();
    if (hud.IsPauseButtonClicked(globalCam.x, globalCam.y))
        UIManager::Get().ShowPause();

    // check for boss fight over sequence completion
    bool bossPresent = false;
    for (const auto& e : enemies)
        if (e.type == EnemyType::Boss) { bossPresent = true; break; }

    if (bossPresent && BossAI_IsFrozen(g_bossAI))
    {
        textScreenMessage = "You Win";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    // periodic state debug log
    static float s_debugTimer = 0.0f;
    s_debugTimer += dt;
    if (s_debugTimer > 1.0f) {
        s_debugTimer = 0.0f;
        float bossHP = -1.0f;
        for (const auto& e : enemies)
            if (e.type == EnemyType::Boss) { bossHP = e.hitPoints; break; }
        std::cout << "[STATE] attackState=" << (int)g_bossAI.attackState
            << " stateTimer=" << g_bossAI.stateTimer
            << " defeatStarted=" << g_bossAI.defeatStarted
            << " hp=" << bossHP << "\n";
    }
}

void BossRoom_Draw()
{
    AESysFrameStart();
    Player& player = ObjectManager::Get().GetPlayer();
    EnvironmentManager::Get().DrawBackground();
    EnvironmentManager::Get().DrawWorld(globalCam.x, globalCam.y, player.weapon, player, 900.0f * 0.5f);
    ObjectManager::Get().Draw(globalCam.x, globalCam.y, 800.0f, 450.0f);
    BossRoom::Get().Draw();
    EnvironmentManager::Get().DrawHUD(globalCam.x, globalCam.y, player.weapon);
    DebugManager::Get().DrawWorldOverlays(globalCam.x, globalCam.y);    
    DebugManager::Get().Draw(globalCam.x, globalCam.y);               
    UIManager::Get().Draw(globalCam.x, globalCam.y);

    if (g_bossRoomFadeAlpha > 0.0f)
    {
        float w = (float)AEGfxGetWindowWidth();
        float h = (float)AEGfxGetWindowHeight();
        MeshManager::Get().DrawSquare(globalCam.x, globalCam.y, w, h, 0, 0, 0, g_bossRoomFadeAlpha);
    }

    // cutscene letterbox bars
    if (!g_cutsceneDone || BossAI_IsFightOver(g_bossAI))
    {
        float w = (float)AEGfxGetWindowWidth();
        float h = (float)AEGfxGetWindowHeight();
        float barH = h * 0.10f;
        float topY = globalCam.y + h * 0.5f - barH * 0.5f;
        MeshManager::Get().DrawSquare(globalCam.x, topY, w, barH, 0, 0, 0, 1.0f);
    }

    AESysFrameEnd();
}

void BossRoom_Free()
{
    EnvironmentManager::Get().SetBossRoomMode(false);

    BossRoom::Get().Free();
}

void BossRoom_Unload()
{
    BossRoom::Get().Unload();
}

void BossRoom::Initialize(const rapidjson::Document& doc)
{
    (void)doc;
}

void BossRoom::Update(float dt)
{
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    Player& player = ObjectManager::Get().GetPlayer();

    for (auto& e : enemies)
    {
        if (e.type != EnemyType::Boss) continue;

        if (!g_cutsceneDone)
        {
            // force lasers off during cutscene
            for (auto& laser : e.bossLasers)
            {
                laser.active        = false;
                laser.state         = BossLaserState::Inactive;
                laser.stateTimer    = 0.0f;
                laser.cooldownTimer = e.laserCooldown;
            }
            continue;
        }

        // force defeat state if HP is zero before AI update overwrites it
        if (e.hitPoints <= 0.0f && !g_bossAI.defeatStarted)
        {
            g_bossAI.defeatStarted = true;
            g_bossAI.phase         = BossPhase::Defeated;
            g_bossAI.attackState   = BossAttackState::FightOver;
            g_bossAI.stateTimer    = 0.0f;
            g_bossAI.lasersEnabled = false;
            e.vel.x = 0.0f;
            e.vel.y = 0.0f;
            e.hitPoints = 0.1f;
            if (e.spriteSheet) e.spriteSheet->Play("fightover");
        }

        BossAI_Update(g_bossAI, e, player, dt);

        bool lasersOn = BossAI_LasersActive(g_bossAI);
        std::cout << "[LASER] lasersOn=" << lasersOn 
            << " attackState=" << (int)g_bossAI.attackState 
            << " lasersEnabled=" << g_bossAI.lasersEnabled
            << " laserCount=" << e.bossLasers.size() << "\n"; // debug
        if (lasersOn)
        {
            BossLasers_Update(e, player, dt);
            CollisionManager::HandleBossLaserCollisions(player, e);
        }
        else
        {
            for (auto& laser : e.bossLasers)
            {
                laser.active        = false;
                laser.state         = BossLaserState::Inactive;
                laser.stateTimer    = 0.0f;
                laser.cooldownTimer = e.laserCooldown;
            }
        }
    }
}

void BossRoom::Draw()
{
    auto& enemies = ObjectManager::Get().GetAllEnemies();
    for (const auto& e : enemies)
        if (e.type == EnemyType::Boss)
            BossLasers_Draw(e);
}

void BossRoom::Free() {}
void BossRoom::Unload() {}
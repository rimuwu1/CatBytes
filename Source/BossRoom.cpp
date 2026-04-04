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
#include "FontManager.h"
#include "ObjectManager.h"
#include "AudioManager.h"
#include "CollisionManager.h"
#include "DebugManager.h"
#include "EnvironmentManager.h"
#include "UIManager.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "Camera.h"
#include "Enemy.h"
#include "WinLose.h"
#include "LevelIndicator.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include "TransitionManager.h"
#include <fstream>
#include <algorithm>
#include <numeric>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

extern Camera globalCam;
extern float camTrauma;
extern float camShakeTime;
static AEAudio g_BossRoomMusic{};
static bool g_BossRoomMusicPlaying = false;
static bool g_BossLaughPlayed = false;
extern AEAudio g_GameMusic;

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

// ============================================================
// monitor state
// ============================================================
static constexpr int MONITOR_COUNT      = 9;
static constexpr int CENTER_MONITOR_IDX = 4;

static Monitor                   g_monitors[MONITOR_COUNT];
static std::string g_monitorTexturePath;
static int                       g_monSheetRows      = 11;
static int                       g_monSheetCols      = 8;
static std::vector<MonitorClipDef> g_monClipDefs;
static AEGfxTexture* g_laserTexture = nullptr;

// teleport sequence before UsePC
enum class BossTeleportState { none, teleporting_away, teleporting_show, done };
static BossTeleportState g_teleportState  = BossTeleportState::none;
static float             g_teleportTimer  = 0.f;

// laser timing (read from boss section of JSON)
static float g_laserTrackTime     = 1.5f;
static float g_laserLockonTime    = 0.8f;
static float g_laserFireTime      = 0.6f;
static float g_laserDamage        = 10.f;
static float g_laserKnockback     = 400.f;
static float g_laserWidth         = 14.f;

// random firing queue
static std::vector<int> g_laserQueue;
static float            g_laserFireInterval  = 2.5f;
static float            g_laserIntervalTimer = 0.f;
static int              g_activeLaserIdx     = -1;

static bool  g_monitorsActivated     = false;
static bool  g_lasersActivatedPlayed = false;
static bool  g_bossAtMonitor         = false;
static float g_bossPlatformX         = 0.f;
static float g_bossPlatformY         = 0.f;
static float g_laserTrackedY = -325.f;

// ============================================================
// dialogue
// ============================================================
static bool  g_bossDialogueActive  = false;
static float g_bossDialogueTimer   = 0.0f;
static const char* g_bossDialogueLine = nullptr;
static bool g_dialogueStarted = false; // true once BossBackIdle has initialized dialogue
bool g_playerDiedBefore = false;

// defeat dialogue -- plays during FightOver loop before disappearing
static bool  g_defeatDialogueShown = false;
static float g_defeatDialogueTimer = 0.0f;
static const char* g_defeatDialogueLine = nullptr;

struct DialoguePair {
    const char* beforeTurn;  // shown while boss faces away
    const char* afterTurn;   // shown after boss turns around
};

static const DialoguePair INTRO_DIALOGUE = {
    "Hm. You actually made it up here. What a surprise.",
    nullptr  // no after-turn line for intro pair 1
};

// intro has 2 beats: one before turn, one spanning turn + fight idle
static const char* INTRO_BEFORE_TURN = "Hm. You actually made it up here. What a surprise.";
static const char* INTRO_AFTER_TURN  = "Still... you're no match for me. Allow me to send you back down.";

// pre-fight ragebait after losing once to boss
static const DialoguePair RETRY_DIALOGUES[] = {
    { "You don't know when to quit, do you?",       "Let me give you a reminder."                          },
    { "Back again?",                                "I'll make this quicker."                              },
    { "It's a shame, really.",                      "You climbed all the way up here just to lose again."  },
    { "You're persistent, I'll give you that.",     "Not that it will save you."                           },
    { "Back again.",                                "Do you really think it will go differently this time?"},
};
// post-fight
static const char* DEFEAT_LINES[] = {
    "...Impossible.",
    "You... actually beat me..",
    "Don't think.. this is over..",
};
static constexpr int RETRY_PAIR_COUNT = 5;
static constexpr int DEFEAT_LINE_COUNT = 3;

static int  g_retryPairIdx    = -1; // set once at BossBackIdle start
static bool g_afterTurnShown  = false;

// ============================================================
// monitor helpers
// ============================================================
static SpriteSheet* MakeMonitorSprite()
{
    if (g_monitorTexturePath.empty()) return nullptr;
    SpriteSheet* ss = new SpriteSheet(g_monitorTexturePath, g_monSheetRows, g_monSheetCols, 93);
    for (const auto& c : g_monClipDefs)
        ss->AddClip(c.name, c.start, c.end, c.duration, c.loop);
    return ss;
}

static void BuildLaserQueue()
{
    g_laserQueue.clear();
    for (int i = 0; i < MONITOR_COUNT; ++i)
        if (!g_monitors[i].isCenter)
            g_laserQueue.push_back(i);
    for (int i = (int)g_laserQueue.size() - 1; i > 0; --i)
        std::swap(g_laserQueue[i], g_laserQueue[rand() % (i + 1)]);
}

static int PopLaserQueue()
{
    if (g_laserQueue.empty()) BuildLaserQueue();
    int idx = g_laserQueue.back();
    g_laserQueue.pop_back();
    return idx;
}

static void TurnMonitorsOff()
{
    for (int i = 0; i < MONITOR_COUNT; ++i)
    {
        if (g_monitors[i].sprite)
            g_monitors[i].sprite->Play("LasersOff");
        g_monitors[i].laserState  = MonitorLaserState::Idle;
        g_monitors[i].laserTimer  = 0.f;
        g_monitors[i].laserLength = 0.f;
    }
    g_monitorsActivated     = false;
    g_lasersActivatedPlayed = false;
    g_activeLaserIdx        = -1;
    g_laserIntervalTimer    = 0.f;
}

static void LoadMonitors(const rapidjson::Document& doc)
{
    if (!doc.HasMember("monitors")) return;
    const auto& monRoot = doc["monitors"];

    g_laserFireInterval = monRoot.HasMember("laser_fire_interval")
        ? (float)monRoot["laser_fire_interval"].GetDouble() : 2.5f;
    g_laserWidth = monRoot.HasMember("laser_width")
        ? (float)monRoot["laser_width"].GetDouble() : 14.f;

    if (doc.HasMember("boss"))
    {
        const auto& b = doc["boss"];
        if (b.HasMember("laser_track"))     g_laserTrackTime  = (float)b["laser_track"].GetDouble();
        if (b.HasMember("laser_lockon"))    g_laserLockonTime = (float)b["laser_lockon"].GetDouble();
        if (b.HasMember("laser_fire"))      g_laserFireTime   = (float)b["laser_fire"].GetDouble();
        if (b.HasMember("laser_damage"))    g_laserDamage     = (float)b["laser_damage"].GetDouble();
        if (b.HasMember("laser_knockback")) g_laserKnockback  = (float)b["laser_knockback"].GetDouble();
    }

    // load spritesheet clips
    g_monClipDefs.clear();
    const auto& ss = monRoot["spritesheet"];
    g_monSheetRows      = ss["rows"].GetInt();
    g_monSheetCols      = ss["cols"].GetInt();
    g_monitorTexturePath = ss["file"].GetString();
    for (const auto& clip : ss["clips"].GetArray())
    {
        MonitorClipDef cd;
        cd.name     = clip["name"].GetString();
        cd.start    = clip["start"].GetInt();
        cd.end      = clip["end"].GetInt();
        cd.duration = (float)clip["duration"].GetDouble();
        cd.loop     = clip["loop"].GetBool();
        g_monClipDefs.push_back(cd);
    }

    // 3x3 grid layout - fallback values, edit in json file
    float centerX   = monRoot.HasMember("center_x")    ? (float)monRoot["center_x"].GetDouble()    : 0.f;
    float centerY   = monRoot.HasMember("center_y")    ? (float)monRoot["center_y"].GetDouble()    : 200.0f;
    float colSpacing = monRoot.HasMember("col_spacing") ? (float)monRoot["col_spacing"].GetDouble() : 400.f;
    float rowSpacing = monRoot.HasMember("row_spacing") ? (float)monRoot["row_spacing"].GetDouble() : 250.f;
    float monW      = monRoot.HasMember("width")        ? (float)monRoot["width"].GetDouble()       : 400.f;
    float monH      = monRoot.HasMember("height")       ? (float)monRoot["height"].GetDouble()      : 250.f;

    const auto& monArray = monRoot["monitors"];
    int count = (int)monArray.Size();
    if (count > MONITOR_COUNT) count = MONITOR_COUNT;

    for (int i = 0; i < count; ++i)
    {
        const auto& entry = monArray[i];
        Monitor& m = g_monitors[i];

        float col = entry.HasMember("col") ? (float)entry["col"].GetDouble() : 0.f;
        float row = entry.HasMember("row") ? (float)entry["row"].GetDouble() : 0.f;

        m.x        = centerX + col * colSpacing;
        m.y        = centerY + row * rowSpacing;
        m.width    = monW;
        m.height   = monH;
        m.isCenter = entry.HasMember("isCenter") && entry["isCenter"].GetBool();
        m.laserState  = MonitorLaserState::Idle;
        m.laserTimer  = 0.f;
        m.laserLength = 0.f;
        m.idleClipName = entry.HasMember("clip") ? entry["clip"].GetString() : "Monitor1";

        delete m.sprite;
        m.sprite = MakeMonitorSprite();
        if (m.sprite) m.sprite->Play(m.idleClipName.c_str());
    }

    // boss teleports behind the center monitor (isCenter == true)
    for (int i = 0; i < count; ++i)
    {
        if (g_monitors[i].isCenter)
        {
            g_bossPlatformX = g_monitors[i].x;
            g_bossPlatformY = 120.f;
            break;
        }
    }

    g_monitorsActivated     = false;
    g_lasersActivatedPlayed = false;
    g_laserTexture = TextureManager::Get().LoadTexture("Assets/Images/laserObstacle.png");
    g_activeLaserIdx        = -1;
    g_laserIntervalTimer    = 0.f;
    BuildLaserQueue();
}

// ============================================================

void BossRoom_Load() 
{
    TransitionManager::GetInstance().Init();
}

void BossRoom_Initialize()
{
    g_dialogueStarted = false;

    g_BossLaughPlayed = false;
    // bossAI initialize
    BossAI_Init(g_bossAI);
    AudioManager::Get().StopAudio(
        AudioManager::Get().GetAudio("elevator_sound")
    );


    // stop previous game music
    AudioManager::Get().StopAudio(g_GameMusic);

    // get audio
    g_BossRoomMusic = AudioManager::Get().GetAudio("boss_room_music");

    // play looping music
    AudioManager::Get().PlayAudio(g_BossRoomMusic, true);
    g_BossRoomMusicPlaying = true;

    // play boss laugh ONCE
    if (!g_BossLaughPlayed)
    {
        AudioManager::Get().PlayAudio(
            AudioManager::Get().GetAudio("boss_laugh"),
            false
        );
        g_BossLaughPlayed = true;
    }
    std::cout << "BossAI:Initialize" << std::endl;

    g_bossRoomFadeAlpha = 1.0f;
    g_bossRoomFadingIn  = true;
    g_cutscenePhase     = CutscenePhase::WalkIn;
    g_cutsceneTimer     = 0.0f;
    g_cutsceneDone      = false;

    // boss dialogue
    g_bossDialogueActive  = false;
    g_bossDialogueTimer   = 0.0f;
    g_bossDialogueLine    = nullptr;
    g_retryPairIdx   = -1;
    g_afterTurnShown = false;
    g_defeatDialogueShown = false;   
    g_defeatDialogueTimer = 0.0f;   
    g_defeatDialogueLine  = nullptr;  

    // reset monitor state
    g_bossAtMonitor         = false;
    g_monitorsActivated     = false;
    g_lasersActivatedPlayed = false;
    g_activeLaserIdx        = -1;

    // clear previous state
    ObjectManager::Get().Clear();
    EnvironmentManager::Get().Clear();

    // Set boss room mode AFTER Clear() since Clear() resets it
    EnvironmentManager::Get().SetBossRoomMode(true);

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

    // load monitors
    LoadMonitors(doc);

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
        UIManager::Get().ShowPause(globalCam.x, globalCam.y);

    TransitionManager::GetInstance().Update(dt);

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

            // clear any active dialogue  
            g_bossDialogueActive = false;
            g_bossDialogueTimer  = 0.0f;
            g_bossDialogueLine   = nullptr;
            g_afterTurnShown     = true; 

            auto& en = ObjectManager::Get().GetAllEnemies();
            for (auto& e : en)
                if (e.type == EnemyType::Boss && e.spriteSheet)
                    e.spriteSheet->Play("attackidle");
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

                // initialize dialogue exactly once on entering this phase
                if (!g_dialogueStarted)
                {
                    std::cout << "[DIALOGUE] g_playerDiedBefore=" << g_playerDiedBefore << "\n"; // debug

                    g_dialogueStarted    = true;
                    g_afterTurnShown     = false;
                    g_bossDialogueActive = false;
                    g_bossDialogueTimer  = 0.0f;

                    if (g_playerDiedBefore)
                    {
                        g_retryPairIdx     = rand() % RETRY_PAIR_COUNT;
                        g_bossDialogueLine = RETRY_DIALOGUES[g_retryPairIdx].beforeTurn;
                    }
                    else
                    {
                        g_retryPairIdx     = -1;
                        g_bossDialogueLine = INTRO_BEFORE_TURN;
                    }
                    g_bossDialogueActive = true;
                    g_bossDialogueTimer  = 0.0f;
                }

                // tick dialogue
                if (g_bossDialogueActive)
                {
                    g_bossDialogueTimer += dt;
                    if (g_bossDialogueTimer >= 3.0f)
                    {
                        g_bossDialogueActive = false;
                        g_bossDialogueTimer  = 0.0f;
                        g_bossDialogueLine   = nullptr;
                    }
                }

                bool readyToTurn = !g_bossDialogueActive && g_cutsceneTimer >= 3.0f;
                if (readyToTurn)
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

                if (turnDone || g_cutsceneTimer >= 1.5f)
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

                // show after-turn line on first frame of this phase
                if (!g_afterTurnShown && !g_bossDialogueActive)
                {
                    g_afterTurnShown = true;
                    const char* afterLine = nullptr;

                    if (g_playerDiedBefore && g_retryPairIdx >= 0 && g_retryPairIdx < RETRY_PAIR_COUNT)
                        afterLine = RETRY_DIALOGUES[g_retryPairIdx].afterTurn;
                    else if (!g_playerDiedBefore)
                        afterLine = INTRO_AFTER_TURN;

                    if (afterLine)
                    {
                        g_bossDialogueLine   = afterLine;
                        g_bossDialogueActive = true;
                        g_bossDialogueTimer  = 0.0f;
                    }
                }

                // tick after-turn dialogue
                if (g_bossDialogueActive)
                {
                    g_bossDialogueTimer += dt;
                    if (g_bossDialogueTimer >= 3.0f)
                    {
                        g_bossDialogueActive = false;
                        g_bossDialogueTimer  = 0.0f;
                        g_bossDialogueLine   = nullptr;
                    }
                }

                // end cutscene once after-turn line is done (or 6s failsafe)
                if (g_afterTurnShown && !g_bossDialogueActive)
                {
                    g_cutsceneDone  = true;
                    g_cutscenePhase = CutscenePhase::Done;

                    for (auto& e : enemies)
                        if (e.type == EnemyType::Boss && e.spriteSheet)
                            e.spriteSheet->Play("attackidle");

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

            // tick monitor sprites during cutscene so they animate
            for (int i = 0; i < MONITOR_COUNT; ++i)
                if (g_monitors[i].sprite) g_monitors[i].sprite->Update(dt);

            // during cutscene: tick animations + camera, no gameplay
            ObjectManager::Get().Update(dt);
            Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
            Camera_Apply(globalCam);
            EnvironmentManager::Get().Update(dt, player, globalCam.y);
            return;

        }
    }

    // noclip - skip collision handling when active
    if (DebugManager::Get().IsNoclipActive()) {
        player.vel.y = 0.0f;
        player.grounded = 1;
        if (AEInputCheckCurr(AEVK_W)) player.pos.y += 400.f * dt;
        if (AEInputCheckCurr(AEVK_S)) player.pos.y -= 400.f * dt;
        if (AEInputCheckCurr(AEVK_A)) player.pos.x -= 400.f * dt;
        if (AEInputCheckCurr(AEVK_D)) player.pos.x += 400.f * dt;
    }
    
    float playerPrevY = player.pos.y;

    ObjectManager::Get().Update(dt);
    BossRoom::Get().Update(dt);
    ParticleManager_Update(dt);

    // noclip - skip collision handling but still rebuild spatial grid
    if (!DebugManager::Get().IsNoclipActive()) {
        ObjectManager::Get().RebuildSpatialGrid();

        CollisionManager::HandleAllCollisionsSpatial(
            player, playerPrevY,
            EnvironmentManager::Get(),
            ObjectManager::Get().GetAllEnemies()
        );
    } else {
        // noclip mode: still rebuild grid for proper enemy updates
        ObjectManager::Get().RebuildSpatialGrid();
    }

    // defeat dialogue -- fires once when FightOver starts, runs outside cutscene block
    for (const auto& e : enemies)
    {
        if (e.type != EnemyType::Boss) continue;
        if (g_bossAI.attackState == BossAttackState::FightOver && !g_defeatDialogueShown)
        {
            g_defeatDialogueShown = true;
            g_defeatDialogueTimer = 0.0f;

            // pick randomly, but track across runs to avoid repeating
            static int s_lastDefeatIdx = -1;
            int idx;
            do {
                idx = rand() % DEFEAT_LINE_COUNT;
            } while (idx == s_lastDefeatIdx && DEFEAT_LINE_COUNT > 1);
            s_lastDefeatIdx = idx;

            g_defeatDialogueLine = DEFEAT_LINES[idx];
        }
    }

    if (g_defeatDialogueLine)
    {
        g_defeatDialogueTimer += dt;
        if (g_defeatDialogueTimer >= 3.0f)
            g_defeatDialogueLine = nullptr;
    }

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

                // boss was knocked out of UsePC — snap back to floor, turn monitors off
                g_bossAtMonitor = false;
                e.pos.x = 0.f;
                e.pos.y = -325.f + e.height * 0.5f;
                e.vel   = { 0.f, 0.f };
                TurnMonitorsOff();
                g_teleportState = BossTeleportState::none;
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

    // Update level indicator manually since EnvironmentManager skips it in boss room mode
    LevelIndicator_Update(dt);

    HUD& hud = EnvironmentManager::Get().GetHUD();
    if (hud.IsPauseButtonClicked(globalCam.x, globalCam.y))
        UIManager::Get().ShowPause(globalCam.x, globalCam.y);

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
    EnvironmentManager::Get().DrawBackground(globalCam.x, globalCam.y);
    BossRoom::Get().Draw();
    EnvironmentManager::Get().DrawWorld(globalCam.x, globalCam.y, player.weapon, player, 900.0f * 0.5f);
    ObjectManager::Get().Draw(globalCam.x, globalCam.y, 800.0f, 450.0f);
    ParticleManager_Draw();
    EnvironmentManager::Get().DrawHUD(globalCam.x, globalCam.y, player.weapon);
    DebugManager::Get().DrawWorldOverlays(globalCam.x, globalCam.y);    
    DebugManager::Get().Draw(globalCam.x, globalCam.y);               
    UIManager::Get().Draw(globalCam.x, globalCam.y);

    TransitionManager::GetInstance().Draw();

    if (g_bossRoomFadeAlpha > 0.0f)
    {
        float w = (float)AEGfxGetWindowWidth();
        float h = (float)AEGfxGetWindowHeight();
        MeshManager::Get().DrawSquare(globalCam.x, globalCam.y, w, h, 0, 0, 0, g_bossRoomFadeAlpha);
    }

    if (g_bossDialogueActive && g_bossDialogueLine)
    {
        s8 font = FontManager::Get().GetMediumFont();
        // draw a dark backing strip
        float w = (float)AEGfxGetWindowWidth();
        MeshManager::Get().DrawSquare(globalCam.x, globalCam.y - 200.0f, w, 70.0f, 0, 0, 0, 0.6f);
        // boss name tag
        FontManager::Get().PrintCentered(font,
            "???",
            -0.72f, -0.50f, 0.55f,
            1.0f, 0.6f, 0.75f, 1.0f);  
        // dialogue line
        FontManager::Get().PrintCentered(font,
            g_bossDialogueLine,
            0.0f, -0.46f, 0.75f,
            1.0f, 1.0f, 1.0f, 1.0f);
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

    // defeat dialogue (separate from cutscene dialogue)
    if (g_defeatDialogueLine)
    {
        s8 font = FontManager::Get().GetMediumFont();
        float w = (float)AEGfxGetWindowWidth();
        MeshManager::Get().DrawSquare(globalCam.x, globalCam.y - 200.0f, w, 70.0f, 0, 0, 0, 0.6f);
        FontManager::Get().PrintCentered(font,
            "???",
            -0.72f, -0.50f, 0.55f,
            1.0f, 0.6f, 0.75f, 1.0f);
        FontManager::Get().PrintCentered(font,
            g_defeatDialogueLine,
            0.0f, -0.46f, 0.75f,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    AESysFrameEnd();
}

void BossRoom_Free()
{
    EnvironmentManager::Get().SetBossRoomMode(false);
    // stop boss music when leaving
    if (g_BossRoomMusicPlaying)
    {
        AudioManager::Get().StopAudio(g_BossRoomMusic);
        g_BossRoomMusicPlaying = false;
    }

    // Reset camera position to prevent popup UI position issues
    globalCam.x = 0.0f;
    globalCam.y = 0.0f;
    AEGfxSetCamPosition(0.0f, 0.0f);

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
            AudioManager::Get().PlayAudio(
                AudioManager::Get().GetAudio("boss_dead"),
                false
            );
            if (e.spriteSheet) e.spriteSheet->Play("fightover");

            // turn monitors off on defeat
            g_bossAtMonitor = false;
            TurnMonitorsOff();
            g_teleportState = BossTeleportState::none;
            g_teleportTimer = 0.f;
        }

        // capture state BEFORE AI update so we can detect transitions
        bool wasInUsePC = (g_bossAI.attackState == BossAttackState::UsePC);

        // skip AI update during teleport sequence — it would overwrite our anim
        bool teleportInProgress = (g_teleportState == BossTeleportState::teleporting_away ||
            g_teleportState == BossTeleportState::teleporting_show);
        if (!teleportInProgress)
            BossAI_Update(g_bossAI, e, player, dt);

        bool nowInUsePC = (g_bossAI.attackState == BossAttackState::UsePC);
        if (teleportInProgress && e.spriteSheet)
            e.spriteSheet->Update(dt);

        if (!wasInUsePC && nowInUsePC && g_teleportState == BossTeleportState::none)
        {
            g_teleportState = BossTeleportState::teleporting_away;
            g_teleportTimer = 0.f;
            e.vel = { 0.f, 0.f };
            if (e.spriteSheet) e.spriteSheet->Play("teleportaway");
        }

        // ── teleport sequence state machine ──────────────────────
        if (nowInUsePC && g_teleportState != BossTeleportState::none && g_teleportState != BossTeleportState::done)
        {
            g_teleportTimer += dt;
            e.vel = { 0.f, 0.f };

            if (g_teleportState == BossTeleportState::teleporting_away)
            {
                // wait for teleportaway anim to finish
                bool animDone = e.spriteSheet && !e.spriteSheet->IsPlaying();
                if (animDone)
                {
                    // snap to center monitor position
                    e.pos.x = g_bossPlatformX;
                    e.pos.y = 120.0f + e.height * 0.5f;
                    e.vel   = { 0.f, 0.f };
                    g_bossAtMonitor = true;
                    g_teleportState = BossTeleportState::teleporting_show;
                    g_teleportTimer = 0.f;
                    if (e.spriteSheet) e.spriteSheet->Play("teleportshow");
                }
            }
            else if (g_teleportState == BossTeleportState::teleporting_show)
            {
                // keep boss pinned at monitor during show anim
                e.pos.x = g_bossPlatformX;
                e.pos.y = 120.0f + e.height * 0.5f;

                bool animDone = e.spriteSheet && !e.spriteSheet->IsPlaying();
                if (animDone)
                {
                    // now start usepc anim and activate monitors
                    if (e.spriteSheet) e.spriteSheet->Play("usepc");
                    g_teleportState = BossTeleportState::done;

                    // center monitor = hacking, all others = LasersActivated
                    if (g_monitors[CENTER_MONITOR_IDX].sprite)
                        g_monitors[CENTER_MONITOR_IDX].sprite->Play("MonitorHacking");
                    for (int i = 0; i < MONITOR_COUNT; ++i)
                        if (!g_monitors[i].isCenter && g_monitors[i].sprite)
                            g_monitors[i].sprite->Play("LasersActivated");

                    g_monitorsActivated     = true;
                    g_lasersActivatedPlayed = true;
                    g_activeLaserIdx        = -1;
                    g_laserIntervalTimer    = 0.f;
                    BuildLaserQueue();
                }
            }
        }

        // ── left UsePC naturally (timed out) ─────────────────────
        if (wasInUsePC && !nowInUsePC && g_monitorsActivated)
        {
            g_bossAtMonitor = false;
            e.pos.x = 0.f;
            e.pos.y = -325.f + e.height * 0.5f;
            e.vel   = { 0.f, 0.f };
            TurnMonitorsOff();
            g_teleportState = BossTeleportState::none;
        }

        // ── keep boss pinned at monitor while UsePC is active ─────
        if (g_bossAtMonitor && nowInUsePC)
        {
            e.pos.x = g_bossPlatformX;
            e.pos.y = 120.0f + e.height * 0.5f;
            e.vel   = { 0.f, 0.f };
        }

        // ── tick monitor sprites ──────────────────────────────────
        for (int i = 0; i < MONITOR_COUNT; ++i)
            if (g_monitors[i].sprite) g_monitors[i].sprite->Update(dt);

        // ── monitor laser logic (only while monitors are active) ──
        if (g_monitorsActivated)
        {
            // transition LasersActivated -> LasersIdle when the one-shot finishes
            for (int i = 0; i < MONITOR_COUNT; ++i)
            {
                Monitor& m = g_monitors[i];
                if (!m.sprite || m.isCenter) continue;
                if (m.sprite->GetCurrentClip() == "LasersActivated" && !m.sprite->IsPlaying())
                    m.sprite->Play("LasersIdle");
            }

            g_laserIntervalTimer += dt;

            // tick the currently active laser monitor's state machine
            if (g_activeLaserIdx >= 0)
            {
                Monitor& active = g_monitors[g_activeLaserIdx];
                active.laserTimer += dt;

                switch (active.laserState)
                {
                case MonitorLaserState::Tracking:
                    // track player X live
                    active.laserTrackedX = player.pos.x;
                    active.laserTrackedY = player.pos.y;
                    if (active.laserTimer >= g_laserTrackTime)
                    {
                        active.laserState = MonitorLaserState::LockedOn;
                        active.laserTimer = 0.f;
                        AudioManager::Get().PlayAudio("laser_off", false);
                    }
                    break;

                case MonitorLaserState::LockedOn:
                    // X is locked; flash telegraph
                    if (active.laserTimer >= g_laserLockonTime)
                    {
                        active.laserState = MonitorLaserState::Firing;
                        active.laserTimer = 0.f;
                        AudioManager::Get().PlayAudio("laser_on", false);
                    }
                    break;

                case MonitorLaserState::Firing:
                {
                    float monBottomY   = active.y;
                    active.laserLength = monBottomY - (-325.f);

                    // check distance from player to the beam line segment (monitor center to floor at locked X)
                    // beam goes from (active.x, active.y) to (active.laserTrackedX, -325.f)
                    float beamDx  = active.laserTrackedX - active.x;
                    float beamDy  = active.laserTrackedY - active.y;
                    float beamLen2 = beamDx * beamDx + beamDy * beamDy;
                    float beamHalfW = g_laserWidth * 0.5f;

                    bool inBeam = false;
                    if (beamLen2 > 0.001f)
                    {
                        // project player position onto beam direction
                        float px = player.pos.x - active.x;
                        float py = player.pos.y - active.y;
                        float t  = (px * beamDx + py * beamDy) / beamLen2;
                        t = (t < 0.f) ? 0.f : (t > 1.f) ? 1.f : t;  // clamp to segment
                        float closestX = active.x + t * beamDx;
                        float closestY = active.y + t * beamDy;
                        float distX    = player.pos.x - closestX;
                        float distY    = player.pos.y - closestY;
                        float dist2    = distX * distX + distY * distY;
                        inBeam = (dist2 <= beamHalfW * beamHalfW);
                    }
                    if (inBeam && !player.isHurt)
                    {
                        player.vel.x = (player.pos.x < active.laserTrackedX ? -1.f : 1.f) * g_laserKnockback;
                        player.vel.y = g_laserKnockback * 0.5f;
                        player.knockbackVel.x = player.vel.x;
                        player.knockbackVel.y = player.vel.y;
                        player.knockbackTimer = player.hurtTimer > 0.0f ? player.hurtTimer : 0.6f;
                        Player_ApplyDamage(player, g_laserDamage);
                    }

                    if (active.laserTimer >= g_laserFireTime)
                    {
                        active.laserLength = 0.f;
                        active.laserState  = MonitorLaserState::Idle;
                        active.laserTimer  = 0.f;
                        g_activeLaserIdx   = -1;
                        g_laserIntervalTimer = 0.f;
                    }
                    break;
                }

                default: break;
                }
            }

            // pick the next monitor to fire once the interval elapses
            if (g_activeLaserIdx == -1 && g_laserIntervalTimer >= g_laserFireInterval)
            {
                // don't start firing until LasersActivated has finished on all monitors
                bool allReady = true;
                for (int i = 0; i < MONITOR_COUNT; ++i)
                    if (!g_monitors[i].isCenter && g_monitors[i].sprite &&
                        g_monitors[i].sprite->GetCurrentClip() == "LasersActivated" &&
                        g_monitors[i].sprite->IsPlaying())
                        allReady = false;

                if (allReady)
                {
                    // skip center monitor — it stays on hacking anim
                    int nextIdx = PopLaserQueue();
                    if (g_monitors[nextIdx].isCenter)
                        nextIdx = PopLaserQueue();
                    g_activeLaserIdx = nextIdx;
                    g_monitors[g_activeLaserIdx].laserState = MonitorLaserState::Tracking;
                    g_monitors[g_activeLaserIdx].laserTimer = 0.f;
                    g_laserIntervalTimer = 0.f;
                }
            }
        }

        // ── original entity laser system ──────────────────────────
        // suppress during UsePC — monitors handle it instead
        bool lasersOn = BossAI_LasersActive(g_bossAI) && !nowInUsePC;
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

    // draw all 9 monitors
    for (int i = 0; i < MONITOR_COUNT; ++i)
    {
        const Monitor& m = g_monitors[i];
        if (!m.sprite) continue;

        MeshManager::Get().DrawSpriteSheet(
            *m.sprite,
            m.x, m.y,
            m.width, m.height,
            1.f, 0.f);

        // draw laser beam / telegraph when this monitor is active
        if (m.laserState == MonitorLaserState::Tracking ||
            m.laserState == MonitorLaserState::LockedOn ||
            m.laserState == MonitorLaserState::Firing)
        {
            float monBottomY = m.y;
            float opacity = (m.laserState == MonitorLaserState::LockedOn) ? 0.7f : 1.f;

            float targetX = m.laserTrackedX;  // live during tracking, frozen during lockedon/firing
            float targetY = m.laserTrackedY;

            float lineW;
            float tileLen;
            int tintR, tintG, tintB;

            if (m.laserState == MonitorLaserState::Tracking)
            {
                lineW = 2.f;
                tileLen = 40.f;
            }
            else if (m.laserState == MonitorLaserState::LockedOn)
            {
                lineW = 5.f;
                tileLen = 40.f;
            }
            else  // firing
            {
                lineW = g_laserWidth;
                tileLen = 40.f;
            }
            tintR = 255; tintG = 255; tintB = 255;

            if (g_laserTexture)
            {
                MeshManager::Get().DrawTexturedLine(
                    g_laserTexture,
                    m.x, monBottomY,
                    targetX, targetY,
                    lineW,
                    tileLen,
                    opacity,
                    tintR, tintG, tintB);
            }
            else
            {
                // fallback to solid color if texture not loaded
                MeshManager::Get().DrawLine(
                    m.x, monBottomY,
                    targetX, targetY,
                    lineW,
                    tintR, tintG, tintB,
                    opacity);
            }
        }
    }

    // draw original entity-based boss lasers (UseWatch state)
    for (const auto& e : enemies)
        if (e.type == EnemyType::Boss)
            BossLasers_Draw(e);
}

void BossRoom::Free()
{
    // clean up monitor spritesheets
    for (int i = 0; i < MONITOR_COUNT; ++i)
    {
        delete g_monitors[i].sprite;
        g_monitors[i].sprite = nullptr;
    }
    g_monClipDefs.clear();
    g_monitorTexturePath.clear();
    g_laserTexture = nullptr;
}

void BossRoom::Unload() {}
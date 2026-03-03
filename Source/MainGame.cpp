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
#include "FileManager.h"
#include "LevelIndicator.h"
#include "Player.h"
#include "enemy.h"
#include "EnemyBullet.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "AudioManager.h"
#include "Audio.h"
#include "UIManager.h"

AEAudio g_GameMusic{};
bool g_GameMusicPlaying = false;
extern AEAudio g_MainMenuMusic;

namespace {
    rapidjson::Document configDoc;   // static to this file

    // ------------------------------------------------------------------------
// Reads GameSave.json if it exists, otherwise falls back to GameConfig.json.
// Clears configDoc first to prevent stale cached values bleeding through.
// ------------------------------------------------------------------------
    static void ParseConfigFromDisk()
    {
        // Wait for any in-flight async save to finish before reading
        while (GameSave::IsSaveInProgress())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

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


// ------------------------------------------------------------------------
// Applies the already-parsed GetConfigDoc() to all managers + resets camera
// ------------------------------------------------------------------------
static void ApplyConfigToManagers()
{
    EnvironmentManager::Get().LoadFromConfig(GetConfigDoc());
    ObjectManager::Get().LoadFromConfig(GetConfigDoc());

    const float ground = -350.0f;
    const float groundHeight = 50.0f;
    const float halfScreenHeight = 900.0f * 0.5f;
    float groundTop = ground + groundHeight * 0.5f;
    Camera_Init(globalCam, ObjectManager::Get().GetPlayer().pos.x, groundTop + halfScreenHeight);
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
    ParseConfigFromDisk();
    ApplyConfigToManagers();
    //Stop main menu music
    AudioManager::Get().StopAudio(g_MainMenuMusic);
    //Start game music (looped)
    g_GameMusic = AudioManager::Get().LoadAudio(Audio::GAME_MUSIC, true);
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
    if(AEInputCheckTriggered(AEVK_ESCAPE)){ //moved from input cos update pause is tweaking
        UIManager::Get().ShowPause();
    }
    float dt = (float)AEFrameRateControllerGetFrameTime();

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

    ObjectManager::Get().Update(dt);

    // ================== COLLISION HANDLING ==================
    auto results = CollisionManager::HandleAllCollisions(
        player,
        playerPrevY,
        EnvironmentManager::Get(),
        ObjectManager::Get().GetAllEnemies(),
        ObjectManager::Get().GetAllEnemyBullets()
    );

    if (results.obstacleHit)
    {
        textScreenMessage = "You Lose";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    // ----- Checkpoint & save -----
    if (EnvironmentManager::Get().HandleCheckpoint(results.checkpointHit))
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

        GameSave::Metadata meta{ "", currentLevel, currentSection, static_cast<int>(ObjectManager::Get().GetPlayerHP()) };
        GameSave::SaveGameAsync(meta, currentLevel, player, enemies, *currentPlatforms);
        GameSave::Notify_Show(GameSave::NotifyType::SAVED);
    }

    if (ObjectManager::Get().IsBossDefeated()) {
        textScreenMessage = "You Win";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    if (!globalCam.debugCam) {
        const float halfScreenHeight = 900.0f * 0.5f;
        float camBottomY = globalCam.y - halfScreenHeight;
        float playerTopY = player.pos.y + player.height * 0.5f;
        if (playerTopY < camBottomY) {
            textScreenMessage = "You Lose";
            GameStateManager::Get().next = GS_WINLOSE;
        }
    }

    if (AEInputCheckTriggered(AEVK_0)) globalCam.debugCam = !globalCam.debugCam;
    if (globalCam.debugCam) Camera_Debug(globalCam, dt);
    else Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
    Camera_Apply(globalCam);

    float backgroundY = globalCam.debugCam ? globalCam.y : player.pos.y;
    EnvironmentManager::Get().Update(dt, player, backgroundY);

    // Pause button -> changes gamestate to pause
    HUD& hud = EnvironmentManager::Get().GetHUD();

    if (hud.IsPauseButtonClicked(globalCam.x, globalCam.y)) {
        UIManager::Get().ShowPause();
        return;
    }

    GameSave::Notify_Update(dt);

    if (AEInputCheckTriggered(AEVK_6)) {
        player.pos.x = 0.0f; player.pos.y = 1900.0f;
        player.vel.x = 0.0f; player.vel.y = 0.0f;
    }
    if (AEInputCheckTriggered(AEVK_7)) {
        player.pos.x = 0.0f; player.pos.y = -300.0f;
        player.vel.x = 0.0f; player.vel.y = 0.0f;
    }

    std::cout << "MainGame:Update" << std::endl;
}

void MainGame_Draw()
{
    AESysFrameStart();

    Player& player = ObjectManager::Get().GetPlayer();
    EnvironmentManager::Get().Draw(globalCam.x, globalCam.y, player.pos.x, player.pos.y);
    ObjectManager::Get().Draw();
    GameSave::Notify_Draw();
    //pop up draw over everything
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
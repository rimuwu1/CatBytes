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

rapidjson::Document configDoc;   // defined elsewhere, extern used if needed

void MainGame_Load()
{
    std::ifstream ifs;
    ifs.open("Assets/Data/GameSave.json");
    if (ifs.is_open()) {
        rapidjson::IStreamWrapper isw(ifs);
        configDoc.ParseStream(isw);
    }
    else {
        ifs.clear();
        ifs.open("Assets/Data/GameConfig.json");
        rapidjson::IStreamWrapper isw(ifs);
        configDoc.ParseStream(isw);
    }
    ifs.close();

    // Load all environment data (platforms, obstacles, checkpoints, buttons)
    EnvironmentManager::Get().LoadFromConfig(configDoc);
    std::cout << "MainGame:Load" << std::endl;
}

void MainGame_Initialize()
{
    EnvironmentManager::Get().Initialize();

    // Load player + level 1 enemies first (this also sets up the player)
    ObjectManager::Get().LoadFromJSON(configDoc["level_1"]);

    // Append enemies from other levels individually (don't re-init player)
    for (auto& enemy : configDoc["level_2"]["enemies"].GetArray())
        ObjectManager::Get().AddEnemyFromJSON(enemy);
    for (auto& enemy : configDoc["level_3"]["enemies"].GetArray())
        ObjectManager::Get().AddEnemyFromJSON(enemy);
    for (auto& enemy : configDoc["level_4"]["enemies"].GetArray())
        ObjectManager::Get().AddEnemyFromJSON(enemy);

    // Bind player to input
    Input_SetPlayer(&ObjectManager::Get().GetPlayer());

    // Camera initial position (ground level)
    const float ground = -350.0f;
    const float groundHeight = 50.0f;
    float groundTop = ground + groundHeight * 0.5f;
    const float halfScreenHeight = 900.0f * 0.5f;
    float cameraStartY = groundTop + halfScreenHeight;
    Camera_Init(globalCam, ObjectManager::Get().GetPlayer().pos.x, cameraStartY);

    std::cout << "MainGame:Initialize" << std::endl;
}

void MainGame_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    Player& player = ObjectManager::Get().GetPlayer();
    std::vector<Enemy>& enemies = ObjectManager::Get().GetAllEnemies();
    std::vector<EnemyBullet>& enemyBullets = ObjectManager::Get().GetAllEnemyBullets();

    // Restart handling
    if (g_resetLevelOnNextUpdate) {
        MainGame_Load();
        MainGame_Initialize();
        g_resetLevelOnNextUpdate = false;
        return;
    }

    float playerPrevY = player.pos.y;

    // Update objects
    ObjectManager::Get().Update(dt);

    // ================== COLLISION HANDLING ==================
    auto results = CollisionManager::HandleAllCollisions(
        player,
        playerPrevY,
        EnvironmentManager::Get(),
        ObjectManager::Get().GetAllEnemies(),
        ObjectManager::Get().GetAllEnemyBullets()
    );

    // React to collision results
    if (results.obstacleHit)
    {
        textScreenMessage = "You Lose";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    // ----- Checkpoint & save -----
    static bool externalSaveRequest = false;  // if needed from UI
    if (EnvironmentManager::Get().HandleCheckpoint(results.checkpointHit, externalSaveRequest))
    {
        int currentSection = EnvironmentManager::Get().GetCurrentSection();
        int currentLevel = currentSection + 1;
        const std::vector<Platform>* currentPlatforms = nullptr;
        switch (currentLevel) {
        case 1: currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
        case 2: currentPlatforms = &EnvironmentManager::Get().GetLevel2Platforms(); break;
        case 3: currentPlatforms = &EnvironmentManager::Get().GetLevel3Platforms(); break;
        case 4: currentPlatforms = &EnvironmentManager::Get().GetBossPlatforms();   break;
        default: currentPlatforms = &EnvironmentManager::Get().GetLevel1Platforms(); break;
        }

        GameSave::Metadata meta{ "1.0", "", currentLevel, 4, 0, 3 };
        GameSave::SaveGameAsync(meta, currentLevel, player, enemies, *currentPlatforms);
        GameSave::Notify_Show(GameSave::NotifyType::SAVED);
    }

    // Check boss death -> win
    if (ObjectManager::Get().IsBossDefeated()) {
        textScreenMessage = "You Win";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    // Fall below camera -> lose
    if (!globalCam.debugCam) {
        const float halfScreenHeight = 900.0f * 0.5f;
        float camBottomY = globalCam.y - halfScreenHeight;
        float playerTopY = player.pos.y + player.height * 0.5f;
        if (playerTopY < camBottomY) {
            textScreenMessage = "You Lose";
            GameStateManager::Get().next = GS_WINLOSE;
        }
    }

    // Camera update
    if (AEInputCheckTriggered(AEVK_0)) globalCam.debugCam = !globalCam.debugCam;
    if (globalCam.debugCam) Camera_Debug(globalCam, dt);
    else Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
    Camera_Apply(globalCam);

    // Update environment (background colour, HUD, level indicator)
    float backgroundY = globalCam.debugCam ? globalCam.y : player.pos.y;
    EnvironmentManager::Get().Update(dt, player, backgroundY);

    // Save notification
    GameSave::Notify_Update(dt);

    // Debug teleports
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

    // Environment draws background, platforms, HUD, level indicator
    EnvironmentManager::Get().Draw(globalCam.x, globalCam.y, player.pos.x, player.pos.y);
    //draw player and enemies
    ObjectManager::Get().Draw();
    // Save notification (on top)
    GameSave::Notify_Draw();

    std::cout << "MainGame:Draw" << std::endl;
    AESysFrameEnd();
}

void MainGame_Free()
{
    Player_Free(ObjectManager::Get().GetPlayer());   // frees player’s bullet list etc.
    ObjectManager::Get().Clear();                    // clears enemies and enemy bullets
    EnvironmentManager::Get().Clear();                // clears platforms, obstacles, etc.
    std::cout << "MainGame:Free" << std::endl;
}

void MainGame_Unload()
{
    std::cout << "MainGame:Unload" << std::endl;
}
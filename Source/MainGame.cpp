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
     kerwinajijie.wong@digipen.edu
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

static EnvironmentManager gEnv;
static bool s_SaveRequested = false;

void MainGame_RequestSave() { s_SaveRequested = true; }

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
    gEnv.LoadFromConfig(configDoc);
    std::cout << "MainGame:Load" << std::endl;
}

void MainGame_Initialize()
{
    gEnv.Initialize();

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

    // Restart handling (if you keep the global flag)

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
    // Ground
    CollisionManager::HandleGround(player, -350.0f, 50.0f, playerPrevY);

    // Platforms (standard when falling)
    CollisionManager::HandlePlatforms(player, playerPrevY, gEnv.GetLevel1Platforms());

    // Landing on any platform (combined correction)
    CollisionManager::HandleLandingOnAnyPlatform(player, playerPrevY,
        gEnv.GetLevel1Platforms(),
        gEnv.GetLevel2Platforms(),
        gEnv.GetLevel3Platforms(),
        gEnv.GetBossPlatforms());

    // Obstacles
    if (CollisionManager::HandleObstacles(player, gEnv.GetObstacles())) {
        textScreenMessage = "You Lose";
        next = GS_WINLOSE;
    }

    // Walls
    CollisionManager::HandleWalls(player, gEnv.GetWallPlatforms());

    // Checkpoints & save
    static bool checkpointSaved = false;
    if (CollisionManager::HandleCheckpoints(player, gEnv.GetCheckpoints()) || s_SaveRequested) {
        if (!checkpointSaved || s_SaveRequested) {
            checkpointSaved = true;
            s_SaveRequested = false;

            int currentSection = gEnv.GetCurrentSection();
            int currentLevel = currentSection + 1;
            std::vector<Platform>* currentPlatforms = nullptr;
            switch (currentLevel) {
            case 1: currentPlatforms = const_cast<std::vector<Platform>*>(&gEnv.GetLevel1Platforms()); break;
            case 2: currentPlatforms = const_cast<std::vector<Platform>*>(&gEnv.GetLevel2Platforms()); break;
            case 3: currentPlatforms = const_cast<std::vector<Platform>*>(&gEnv.GetLevel3Platforms()); break;
            case 4: currentPlatforms = const_cast<std::vector<Platform>*>(&gEnv.GetBossPlatforms());   break;
            default: currentPlatforms = const_cast<std::vector<Platform>*>(&gEnv.GetLevel1Platforms()); break;
            }

            GameSave::Metadata meta{ "1.0", "", currentLevel, 4, 0, 3 };
            GameSave::SaveGame(meta, currentLevel, player, enemies, *currentPlatforms);
            GameSave::Notify_Show(GameSave::NotifyType::SAVED);
        }
    }
    else {
        checkpointSaved = false;
    }

    // Level 2 buttons
    auto& buttons = gEnv.GetLevel2Buttons();
    auto& level2Plats = const_cast<std::vector<Platform>&>(gEnv.GetLevel2Platforms());
    CollisionManager::HandleButtons(player, buttons, level2Plats);

    // Enemy collisions
    CollisionManager::HandlePlayerEnemyCollisions(player, enemies);
    CollisionManager::HandleEnemyBulletPlayerCollisions(enemyBullets, player);
    CollisionManager::HandlePlayerBulletEnemyCollisions(player, enemies);
    CollisionManager::HandlePlayerMeleeEnemyCollisions(player, enemies);

    // Enemy shooting (easy enemies)
    for (auto& enemy : enemies) {
        if (!enemy.isAlive) continue;
        if (enemy.type == EnemyType::Easy) {
            enemy.shootTimer -= dt;
            if (enemy.shootTimer <= 0.0f) {
                ObjectManager::Get().SpawnEnemyBullet(enemy,
                    enemy.bulletSpeed,
                    enemy.bulletDamage,
                    1600.0f);
                enemy.shootTimer = enemy.shootCooldown;
            }
        }
    }

    // Check boss death ? win
    bool bossExists = false, bossDead = true;
    for (auto& enemy : enemies) {
        if (enemy.type == EnemyType::Boss) {
            bossExists = true;
            if (enemy.isAlive) bossDead = false;
        }
    }
    if (bossExists && bossDead) {
        textScreenMessage = "You Win";
        next = GS_WINLOSE;
    }

    // Fall below camera ? lose
    if (!globalCam.debugCam) {
        const float halfScreenHeight = 900.0f * 0.5f;
        float camBottomY = globalCam.y - halfScreenHeight;
        float playerTopY = player.pos.y + player.height * 0.5f;
        if (playerTopY < camBottomY) {
            textScreenMessage = "You Lose";
            next = GS_WINLOSE;
        }
    }

    // Camera update
    if (AEInputCheckTriggered(AEVK_0)) globalCam.debugCam = !globalCam.debugCam;
    if (globalCam.debugCam) Camera_Debug(globalCam, dt);
    else Camera_FollowPlayer(globalCam, player.pos.x, player.pos.y, dt);
    Camera_Apply(globalCam);

    // Update environment (background colour, HUD, level indicator)
    float backgroundY = globalCam.debugCam ? globalCam.y : player.pos.y;
    gEnv.Update(dt, player, backgroundY);

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

    // Environment draws background, platforms, HUD, minimap, level indicator
    gEnv.Draw(globalCam.x, globalCam.y, player.pos.x, player.pos.y);

    // Draw enemies
    for (const auto& enemy : ObjectManager::Get().GetAllEnemies()) {
        if (enemy.isAlive) Enemy_Draw(enemy);
    }

    // Draw enemy bullets
    for (const auto& bullet : ObjectManager::Get().GetAllEnemyBullets()) {
        if (bullet.active) EnemyBullet_Draw(bullet);
    }

    // Low?HP overlays
    for (const auto& enemy : ObjectManager::Get().GetAllEnemies()) {
        if (!enemy.isAlive) continue;
        float overlayAlpha = 0.0f;
        if (enemy.hitStunTimer > 0.0f)
            overlayAlpha = enemy.hitStunTimer / 0.5f;
        else if (enemy.hitPoints == 1)
            overlayAlpha = 0.25f;
        if (overlayAlpha > 0.0f && enemy.lowHpTexture) {
            MeshManager::Get().DrawTexturedSquare(
                enemy.lowHpTexture,
                enemy.pos.x, enemy.pos.y,
                enemy.width, enemy.height,
                overlayAlpha
            );
        }
    }

    // Draw player
    Player_Draw(player);

    // Save notification (on top)
    GameSave::Notify_Draw();

    std::cout << "MainGame:Draw" << std::endl;
    AESysFrameEnd();
}

void MainGame_Free()
{
    Player_Free(ObjectManager::Get().GetPlayer());
    gEnv.Free();
    std::cout << "MainGame:Free" << std::endl;
}

void MainGame_Unload()
{
    PlayerBullet_FreeShared();
    gEnv.Unload();
    std::cout << "MainGame:Unload" << std::endl;
}
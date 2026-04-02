/* Start Header ************************************************************************/
/*!
\file GameSaveManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Class that handles saving/loading game state (player, enemies, platforms)
       and displays save/reset notifications.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of Digipen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "pch.h"
#include "Player.h"
#include "Enemy.h"
#include "Platforms.h"
#include "Buff.h"
#include <cfloat>
#include <vector>
#include <string>
#include <atomic>

class GameSaveManager
{
public:
    enum class NotifyType { NONE, SAVED, RESET };

    // ----- Save / Load data structures ------------------------------------
    struct Metadata {
        std::string save_date;
        int current_level;
        int levels_completed;
        int player_lives;
    };

    struct PlayerSaveData {
        float x, y, hp;
        int weapon;
        std::vector<Buff> buffs;
    };

    struct EnemySaveData {
        float x, platformY;
        float hp;
    };

    struct BuffSaveData {
        int type;
        float x, y;
    };

    // ----- Notification control -------------------------------------------
    static void        Notify_Show(NotifyType type);   // call after SaveGame / ResetSave
    static void        Notify_Update(float dt);
    static void        Notify_Draw();                  // call from LevelIndicator_Draw
    static NotifyType  Notify_GetCurrent();

    // ----- Save game (async) ----------------------------------------------
    static bool IsSaveInProgress();
    // Block until any in-progress async save completes. Uses a condition variable.
    static void WaitForSaveToFinish();

    // Asynchronous save - does not block the caller.
    static void SaveGameAsync(
        const Metadata& metadata,
        int                        currentLevel,
        const Player& player,
        const std::vector<Enemy>& enemies,
        const std::vector<Platform>& platforms,
        const std::vector<Buff>& worldBuffs,
        const std::vector<Platform>& toggleWalls = {},
        float levelMinY = -FLT_MAX,
        float levelMaxY = FLT_MAX,
        const std::string& filepath = "Assets/Data/GameSave.json");

    // ----- Reset save file (copy config over save) -----------------------
    static void ResetSave(
        const std::string& configPath = "Assets/Data/GameConfig.json",
        const std::string& savePath = "Assets/Data/GameSave.json");

private:
    // Internal save routine (runs on worker thread)
    static void SaveGame_Internal(
        const Metadata& metadata,
        int                        currentLevel,
        const PlayerSaveData& player,
        const std::vector<EnemySaveData>& enemies,
        const std::vector<Platform>& platforms,
        const std::vector<Platform>& toggleWalls,
        const std::vector<BuffSaveData>& worldBuffs,
        const std::string& filepath);

    // Helper to extract minimal data for threading
    static PlayerSaveData ExtractPlayerData(const Player& p);
    static std::vector<EnemySaveData> ExtractEnemyData(
        const std::vector<Enemy>& enemies,
        float levelMinY = -FLT_MAX,
        float levelMaxY = FLT_MAX);
    static std::vector<BuffSaveData> ExtractBuffData(const std::vector<Buff>& buffs);

    // ----- Static members -------------------------------------------------
    static std::atomic<bool>  s_SaveInProgress;
    static NotifyType         s_NotifyType;
    static float              s_NotifyTimer;
    static const float        NOTIFY_DURATION;
    static const float        NOTIFY_FADE;
};

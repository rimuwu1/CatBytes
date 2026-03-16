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
#include "enemy.h"
#include "Platforms.h"
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
        float x, y;
        float hp;
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
        const std::string& filepath);

    // Helper to extract minimal data for threading
    static PlayerSaveData ExtractPlayerData(const Player& p);
    static std::vector<EnemySaveData> ExtractEnemyData(const std::vector<Enemy>& enemies);

    // ----- Static members -------------------------------------------------
    static std::atomic<bool>  s_SaveInProgress;
    static NotifyType         s_NotifyType;
    static float              s_NotifyTimer;
    static const float        NOTIFY_DURATION;
    static const float        NOTIFY_FADE;
};

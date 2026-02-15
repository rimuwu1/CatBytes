#pragma once
#include "pch.h"
#include "Player.h"
#include "enemy.h"
#include <vector>
#include <string>

namespace GameSave
{

    enum class NotifyType { NONE, SAVED, RESET };

    void        Notify_Show(NotifyType type);   // call after SaveGame / ResetSave
    void        Notify_Update(float dt);
    void        Notify_Draw();                  // call from LevelIndicator_Draw
    NotifyType  Notify_GetCurrent();

    struct Metadata {
        std::string game_version;
        std::string save_date;
        int current_level;
        int total_levels;
        int levels_completed;
        int player_lives;
    };

    // Saves metadata, player state, and enemy state for the given level.
    // Platforms, walls, and obstacles are preserved from the existing file.
    void SaveGame(
        const Metadata& metadata,
        int                        currentLevel,
        const Player& player,
        const std::vector<Enemy>& enemies,
        const std::string& filepath = "Assets/Data/GameSave.json");

    void ResetSave(const std::string& configPath = "Assets/Data/GameConfig.json",
        const std::string& savePath = "Assets/Data/GameSave.json");
}
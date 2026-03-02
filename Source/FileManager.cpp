/* Start Header ************************************************************************/
/*!
\file FileManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Implements the Namespace Gamesave. Saves hp, position and metadata.
       reset function available which overrides the save file with the config file

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "FileManager.h"
#include "Player.h"
#include "Platforms.h"
#include "enemy.h"
#include "Fonts.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <atomic>
#include <mutex>


namespace GameSave
{
    std::atomic<bool> g_SaveInProgress{ false };

    bool IsSaveInProgress() {
        return g_SaveInProgress;
    }
    // -------------------------------------------------------------------------
    // Helper: get current datetime as "DD-MM-YYYY HH:MM:SS"
    // -------------------------------------------------------------------------
    static std::string GetCurrentDateTimeString()
    {
        std::time_t now = std::time(nullptr);
        struct tm tinfo = { 0 }; // initialize to zero

        if (localtime_s(&tinfo, &now) == 0)
        {
            char buf[32];
            std::strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tinfo);
            return std::string(buf);
        }
        // Fallback in case of error
        return "00-00-0000 00:00:00";
    }

    // Convert before spawning the thread
    static PlayerSaveData ExtractPlayerData(const Player& p) {
        return { p.pos.x, p.pos.y, p.hp };
    }

    static std::vector<EnemySaveData> ExtractEnemyData(const std::vector<Enemy>& enemies) {
        std::vector<EnemySaveData> out;
        out.reserve(enemies.size());
        for (const auto& e : enemies) {
            GameSave::EnemySaveData data{};
            data.x = e.pos.x;
            data.y = e.pos.y;
            data.hp = e.hitPoints;
            out.push_back(data);
        }
        return out;
    }

    // -------------------------------------------------------------------------
    // SaveGame
    //
    // Saves metadata + the current level's player hp/pos and enemy hp/pos.
    // -------------------------------------------------------------------------
    void SaveGame_Internal(
        const Metadata& metadata,
        int                        currentLevel,
        const PlayerSaveData& player,      // lightweight
        const std::vector<EnemySaveData>& enemies,   // lightweight
        const std::vector<Platform>& platforms,
        const std::string& filepath)
    {
        // ------------------------------------------------------------------
        // 1. Load the static configuration (GameConfig.json) as the base document
        // ------------------------------------------------------------------
        rapidjson::Document doc(rapidjson::kObjectType);
        {
            std::ifstream configFile("Assets/Data/GameConfig.json");
            if (!configFile.is_open())
            {
                std::cout << "[SaveGame] ERROR: Could not open GameConfig.json\n";
                return;
            }
            std::string content(
                (std::istreambuf_iterator<char>(configFile)),
                std::istreambuf_iterator<char>());
            configFile.close();

            if (doc.Parse(content.c_str()).HasParseError())
            {
                std::cout << "[SaveGame] ERROR: Failed to parse GameConfig.json\n";
                return;
            }
        }

        // ------------------------------------------------------------------
        // 2. Update metadata (dynamic fields only)
        // ------------------------------------------------------------------
        if (doc.HasMember("metadata") && doc["metadata"].IsObject())
        {
            rapidjson::Value& meta = doc["metadata"];
            // save_date is always regenerated
            meta["save_date"] = rapidjson::Value(GetCurrentDateTimeString().c_str(), doc.GetAllocator());
            // update other dynamic metadata from the passed structure
            meta["current_level"] = metadata.current_level;
            meta["levels_completed"] = metadata.levels_completed;
            meta["player_lives"] = metadata.player_lives;
            // game_version and total_levels remain as in config (static)
        }

        // ------------------------------------------------------------------
        // 3. Update player position and HP
        // ------------------------------------------------------------------
        if (doc.HasMember("player") && doc["player"].IsObject()) {
            rapidjson::Value& playerObj = doc["player"];
            playerObj["x"] = player.x;
            playerObj["y"] = player.y;
            playerObj["hp"] = player.hp;
        }

        // ------------------------------------------------------------------
        // 4. Set "completed" flag for all levels based on levels_completed
        // ------------------------------------------------------------------
        std::string levelPrefix = "level_";
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            if (it->name.GetStringLength() > levelPrefix.size() &&
                strncmp(it->name.GetString(), levelPrefix.c_str(), levelPrefix.size()) == 0)
            {
                // Extract level number
                int lvl = std::atoi(it->name.GetString() + levelPrefix.size());
                bool completed = (lvl <= metadata.levels_completed);
                if (it->value.IsObject())
                {
                    rapidjson::Value& lvlObj = it->value;
                    if (!lvlObj.HasMember("completed"))
                        lvlObj.AddMember("completed", completed, doc.GetAllocator());
                    else
                        lvlObj["completed"] = completed;
                }
            }
        }

        // ------------------------------------------------------------------
        // 5. Update dynamic data for the current level
        // ------------------------------------------------------------------
        std::string levelKey = "level_" + std::to_string(currentLevel);
        if (doc.HasMember(levelKey.c_str()) && doc[levelKey.c_str()].IsObject())
        {
            rapidjson::Value& levelObj = doc[levelKey.c_str()];

            // Enemies: update x, y, hp (index‑matched)
            if (levelObj.HasMember("enemies") && levelObj["enemies"].IsArray())
            {
                rapidjson::Value& enemiesArr = levelObj["enemies"];
                for (rapidjson::SizeType i = 0; i < enemiesArr.Size(); ++i)
                {
                    if (i >= static_cast<rapidjson::SizeType>(enemies.size())) break;
                    enemiesArr[i]["x"] = enemies[i].x;
                    enemiesArr[i]["y"] = enemies[i].y;
                    enemiesArr[i]["hp"] = enemies[i].hp;
                    // all other enemy properties remain as in config
                }
            }

            // Platforms: add/update active state
            if (levelObj.HasMember("platforms") && levelObj["platforms"].IsArray())
            {
                rapidjson::Value& platformsArr = levelObj["platforms"];
                if (platformsArr.Size() == static_cast<rapidjson::SizeType>(platforms.size()))
                {
                    for (rapidjson::SizeType i = 0; i < platformsArr.Size(); ++i)
                    {
                        if (!platformsArr[i].IsObject()) continue;
                        if (!platformsArr[i].HasMember("active"))
                            platformsArr[i].AddMember("active", platforms[i].active, doc.GetAllocator());
                        else
                            platformsArr[i]["active"] = platforms[i].active;
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // 6. Serialize and write to the save file
        // ------------------------------------------------------------------
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 4);
        doc.Accept(writer);

        std::ofstream outFile(filepath);
        if (!outFile.is_open())
        {
            std::cout << "[SaveGame] ERROR: Could not open save file: " << filepath << std::endl;
            return;
        }
        outFile << buffer.GetString();
        outFile.close();
    }

    //async wrapper to run the save on a different thread (prevent freezing)
    void SaveGameAsync(
        const Metadata& metadata,
        int                          currentLevel,
        const Player& player,     // still takes Player by ref
        const std::vector<Enemy>& enemies,
        const std::vector<Platform>& platforms,
        const std::string& filepath)
    {
        if (g_SaveInProgress.load()) return;
        g_SaveInProgress.store(true);

        // Extract only what's needed - these are trivially copyable
        PlayerSaveData              playerData = ExtractPlayerData(player);
        std::vector<EnemySaveData>  enemyData = ExtractEnemyData(enemies);
        std::vector<Platform>       platCopy = platforms;

        std::thread([=]() mutable {
            SaveGame_Internal(metadata, currentLevel,
                playerData, enemyData, platCopy, filepath);
            g_SaveInProgress.store(false);
            }).detach();
    }

    void ResetSave(const std::string& configPath,
        const std::string& savePath)
    {
        std::ifstream src(configPath, std::ios::binary);
        if (!src.is_open())
        {
            std::cout << "[ResetSave] ERROR: Could not open config: " << configPath << std::endl;
            return;
        }

        std::ofstream dst(savePath, std::ios::binary | std::ios::trunc);
        if (!dst.is_open())
        {
            std::cout << "[ResetSave] ERROR: Could not open save: " << savePath << std::endl;
            return;
        }

        dst << src.rdbuf(); // copies the entire file in one line
    }

} // namespace GameSave

// -------------------------------------------------------------------------
// Save notification state
// -------------------------------------------------------------------------
static GameSave::NotifyType s_NotifyType = GameSave::NotifyType::NONE;
static float                s_NotifyTimer = 0.0f;
static const float          NOTIFY_DURATION = 2.0f;
static const float          NOTIFY_FADE = 0.5f;

void GameSave::Notify_Show(NotifyType type)
{
    s_NotifyType = type;
    s_NotifyTimer = 0.0f;
}

void GameSave::Notify_Update(float dt)
{
    if (s_NotifyType == NotifyType::NONE) return;
    s_NotifyTimer += dt;
    if (s_NotifyTimer >= NOTIFY_DURATION)
        s_NotifyType = NotifyType::NONE;
}

GameSave::NotifyType GameSave::Notify_GetCurrent()
{
    return s_NotifyType;
}

void GameSave::Notify_Draw()
{
    if (s_NotifyType == NotifyType::NONE) return;

    const char* text = (s_NotifyType == NotifyType::SAVED) ? "Game Saved" : "Save Deleted";

    // fade in / out
    float a = 1.0f;
    if (s_NotifyTimer < NOTIFY_FADE)
        a = AEClamp(s_NotifyTimer / NOTIFY_FADE, 0.0f, 1.0f);
    else if (s_NotifyTimer > NOTIFY_DURATION - NOTIFY_FADE)
        a = AEClamp((NOTIFY_DURATION - s_NotifyTimer) / NOTIFY_FADE, 0.0f, 1.0f);

    // drawn slightly below the level indicator so they don't overlap
    AEGfxPrint(g_FontMedium, text, 0.0f, 0.50f, 0.8f, 1.0f, 1.0f, 1.0f, a);
}

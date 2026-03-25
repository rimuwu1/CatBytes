/* Start Header ************************************************************************/
/*!
\file GameSaveManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Implementation of GameSaveManager class.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of Digipen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "GameSaveManager.h"
#include "Fonts.h"                      // for g_FontMedium
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
#include <condition_variable>

// ----- Static member definitions ------------------------------------------------
std::atomic<bool> GameSaveManager::s_SaveInProgress{ false };
GameSaveManager::NotifyType GameSaveManager::s_NotifyType = NotifyType::NONE;
float GameSaveManager::s_NotifyTimer = 0.0f;
const float GameSaveManager::NOTIFY_DURATION = 2.0f;
const float GameSaveManager::NOTIFY_FADE = 0.5f;
// Synchronization for waiting on save completion
static std::mutex s_SaveMutex;
static std::condition_variable s_SaveCv;

namespace {
    // -------------------------------------------------------------------------
    // File‑local helpers (not part of the class)
    // -------------------------------------------------------------------------
    std::string GetCurrentDateTimeString()
    {
        std::time_t now = std::time(nullptr);
        struct tm tinfo = { 0 };
        if (localtime_s(&tinfo, &now) == 0)
        {
            char buf[32];
            std::strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &tinfo);
            return std::string(buf);
        }
        return "00-00-0000 00:00:00";
    }
} // anonymous namespace

// ----- Public static methods ----------------------------------------------------
bool GameSaveManager::IsSaveInProgress()
{
    return s_SaveInProgress.load();
}

void GameSaveManager::WaitForSaveToFinish()
{
    // Fast path
    if (!s_SaveInProgress.load()) return;

    std::unique_lock<std::mutex> lk(s_SaveMutex);
    s_SaveCv.wait(lk, []() { return !s_SaveInProgress.load(); });
}

void GameSaveManager::Notify_Show(NotifyType type)
{
    s_NotifyType = type;
    s_NotifyTimer = 0.0f;
}

void GameSaveManager::Notify_Update(float dt)
{
    if (s_NotifyType == NotifyType::NONE) return;
    s_NotifyTimer += dt;
    if (s_NotifyTimer >= NOTIFY_DURATION)
        s_NotifyType = NotifyType::NONE;
}

GameSaveManager::NotifyType GameSaveManager::Notify_GetCurrent()
{
    return s_NotifyType;
}

void GameSaveManager::Notify_Draw()
{
    if (s_NotifyType == NotifyType::NONE) return;

    const char* text = (s_NotifyType == NotifyType::SAVED) ? "Game Saved" : "Save Deleted";

    // fade in / out
    float a = 1.0f;
    if (s_NotifyTimer < NOTIFY_FADE)
        a = AEClamp(s_NotifyTimer / NOTIFY_FADE, 0.0f, 1.0f);
    else if (s_NotifyTimer > NOTIFY_DURATION - NOTIFY_FADE)
        a = AEClamp((NOTIFY_DURATION - s_NotifyTimer) / NOTIFY_FADE, 0.0f, 1.0f);

    AEGfxPrint(g_FontMedium, text, 0.0f, 0.50f, 0.8f, 1.0f, 1.0f, 1.0f, a);
}

// ----- Save game implementation ------------------------------------------------
void GameSaveManager::SaveGameAsync(
    const Metadata& metadata,
    int                        currentLevel,
    const Player& player,
    const std::vector<Enemy>& enemies,
    const std::vector<Platform>& platforms,
    const std::vector<Buff>& worldBuffs,
    const std::vector<Platform>& toggleWalls,
    float levelMinY,
    float levelMaxY,
    const std::string& filepath)
{
    if (s_SaveInProgress.load()) return;
    s_SaveInProgress.store(true);

    // Extract only trivially copyable data
    PlayerSaveData              playerData = ExtractPlayerData(player);
    std::vector<EnemySaveData>  enemyData = ExtractEnemyData(enemies, levelMinY, levelMaxY);
    std::vector<BuffSaveData>   buffData = ExtractBuffData(worldBuffs);
    std::vector<Platform>       platCopy = platforms;
    std::vector<Platform>       toggleWallCopy = toggleWalls;

    std::thread([=]() mutable {
        SaveGame_Internal(metadata, currentLevel,
            playerData, enemyData, platCopy, toggleWallCopy, buffData, filepath);
        // set flag under lock then notify to avoid missed wakeups
        {
            std::lock_guard<std::mutex> lk(s_SaveMutex);
            s_SaveInProgress.store(false);
        }
        s_SaveCv.notify_all();
        }).detach();
}

void GameSaveManager::ResetSave(const std::string& configPath, const std::string& savePath)
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

    dst << src.rdbuf();
}

// ----- Private static methods --------------------------------------------------
void GameSaveManager::SaveGame_Internal(
    const Metadata& metadata,
    int                        currentLevel,
    const PlayerSaveData& player,
    const std::vector<EnemySaveData>& enemies,
    const std::vector<Platform>& platforms,
    const std::vector<Platform>& toggleWalls,
    const std::vector<BuffSaveData>& worldBuffs,
    const std::string& filepath)
{
    // 1. Load the static configuration (GameConfig.json) as the base document
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

    // 2. Update metadata
    if (doc.HasMember("metadata") && doc["metadata"].IsObject())
    {
        rapidjson::Value& meta = doc["metadata"];
        meta["save_date"] = rapidjson::Value(GetCurrentDateTimeString().c_str(), doc.GetAllocator());
        meta["current_level"] = metadata.current_level;
        meta["levels_completed"] = metadata.levels_completed;
        meta["player_lives"] = metadata.player_lives;
    }

    // 3. Update player
    if (doc.HasMember("player") && doc["player"].IsObject())
    {
        rapidjson::Value& playerObj = doc["player"];
        auto& alloc = doc.GetAllocator();
        playerObj["x"] = player.x;
        playerObj["y"] = player.y;
        playerObj["hp"] = player.hp;
        // weapon — update in place if exists, add if not
        if (playerObj.HasMember("weapon"))
            playerObj["weapon"] = static_cast<int>(player.weapon);
        else
            playerObj.AddMember("weapon", static_cast<int>(player.weapon), alloc);
        
        // buffs — always remove then re-add to avoid duplicate arrays
        if (playerObj.HasMember("buffs"))
            playerObj.RemoveMember("buffs");
        rapidjson::Value buffsArr(rapidjson::kArrayType);
        for (const auto& buff : player.buffs) {
            if (!buff.active) continue;
            rapidjson::Value b(rapidjson::kObjectType);
            b.AddMember("type", static_cast<int>(buff.type), alloc);
            buffsArr.PushBack(b, alloc);
        }
        playerObj.AddMember("buffs", buffsArr, alloc);
    }

    // 4. Set "completed" flags for all levels
    std::string levelPrefix = "level_";
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
        if (it->name.GetStringLength() > levelPrefix.size() &&
            strncmp(it->name.GetString(), levelPrefix.c_str(), levelPrefix.size()) == 0)
        {
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

    // 5. Update dynamic data for the current level
    std::string levelKey = "level_" + std::to_string(currentLevel);
    if (doc.HasMember(levelKey.c_str()) && doc[levelKey.c_str()].IsObject())
    {
        rapidjson::Value& levelObj = doc[levelKey.c_str()];

        // Enemies
        if (levelObj.HasMember("enemies") && levelObj["enemies"].IsArray())
        {
            rapidjson::Value& enemiesArr = levelObj["enemies"];
            auto& alloc = doc.GetAllocator();
            for (rapidjson::SizeType i = 0; i < enemiesArr.Size(); ++i)
            {
                if (i >= static_cast<rapidjson::SizeType>(enemies.size())) break;
                if (!enemiesArr[i].IsObject()) continue;
                
                // Safely update or add x, platform_y, hp fields
                if (enemiesArr[i].HasMember("x"))
                    enemiesArr[i]["x"] = enemies[i].x;
                else
                    enemiesArr[i].AddMember("x", enemies[i].x, alloc);
                
                // Always use platform_y for consistency
                if (enemiesArr[i].HasMember("platform_y"))
                    enemiesArr[i]["platform_y"] = enemies[i].platformY;
                else if (enemiesArr[i].HasMember("y")) {
                    // Convert old "y" to "platform_y"
                    enemiesArr[i].RemoveMember("y");
                    enemiesArr[i].AddMember("platform_y", enemies[i].platformY, alloc);
                }
                else
                    enemiesArr[i].AddMember("platform_y", enemies[i].platformY, alloc);
                
                if (enemiesArr[i].HasMember("hp"))
                    enemiesArr[i]["hp"] = enemies[i].hp;
                else
                    enemiesArr[i].AddMember("hp", enemies[i].hp, alloc);
            }
        }

        // Platforms – active state
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

        // Toggleable walls – active state (level 3 only currently)
        if (levelObj.HasMember("toggleable_walls") && levelObj["toggleable_walls"].IsArray())
        {
            rapidjson::Value& wallsArr = levelObj["toggleable_walls"];
            if (wallsArr.Size() == static_cast<rapidjson::SizeType>(toggleWalls.size()))
            {
                for (rapidjson::SizeType i = 0; i < wallsArr.Size(); ++i)
                {
                    if (!wallsArr[i].IsObject()) continue;
                    if (!wallsArr[i].HasMember("active"))
                        wallsArr[i].AddMember("active", toggleWalls[i].active, doc.GetAllocator());
                    else
                        wallsArr[i]["active"] = toggleWalls[i].active;
                }
            }
        }

        // World buffs - save active buffs on the map
        if (levelObj.HasMember("buffs"))
            levelObj.RemoveMember("buffs");
        rapidjson::Value buffsArr(rapidjson::kArrayType);
        for (const auto& buff : worldBuffs) {
            rapidjson::Value b(rapidjson::kObjectType);
            // Convert type int back to string for JSON
            const char* typeStr = "none";
            switch (static_cast<BuffType>(buff.type)) {
            case BuffType::SHIELD:  typeStr = "shield";  break;
            case BuffType::FULL_HP: typeStr = "full_hp"; break;
            case BuffType::DASH:    typeStr = "dash";    break;
            default: break;
            }
            b.AddMember("type", rapidjson::Value(typeStr, doc.GetAllocator()), doc.GetAllocator());
            b.AddMember("x", buff.x, doc.GetAllocator());
            b.AddMember("y", buff.y, doc.GetAllocator());
            buffsArr.PushBack(b, doc.GetAllocator());
        }
        levelObj.AddMember("buffs", buffsArr, doc.GetAllocator());
    }

    // 6. Write to file
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

GameSaveManager::PlayerSaveData GameSaveManager::ExtractPlayerData(const Player& p)
{
    return { p.pos.x, p.pos.y, p.hp, static_cast<int>(p.weapon), p.buffs };
}

std::vector<GameSaveManager::EnemySaveData> GameSaveManager::ExtractEnemyData(
    const std::vector<Enemy>& enemies, float levelMinY, float levelMaxY)
{
    std::vector<EnemySaveData> out;
    for (const auto& e : enemies) {
        if (e.pos.y >= levelMinY && e.pos.y <= levelMaxY)
            out.push_back({ e.pos.x, e.platformY, e.hitPoints });
    }
    return out;
}

std::vector<GameSaveManager::BuffSaveData> GameSaveManager::ExtractBuffData(
    const std::vector<Buff>& buffs)
{
    std::vector<BuffSaveData> out;
    for (const auto& buff : buffs) {
        if (buff.active)
            out.push_back({ static_cast<int>(buff.type), buff.pos.x, buff.pos.y });
    }
    return out;
}

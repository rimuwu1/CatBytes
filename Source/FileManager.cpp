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
#include "enemy.h"
#include "Fonts.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>
#include <string>
#include <vector>
#include <ctime>



namespace GameSave
{

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
        return "01-01-1970 00:00:00";
    }

    // -------------------------------------------------------------------------
    // SaveGame
    //
    // Saves metadata + the current level's player hp/pos and enemy hp/pos.
    // -------------------------------------------------------------------------
    void SaveGame(
        const Metadata& metadata,
        int                        currentLevel,
        const Player& player,
        const std::vector<Enemy>& enemies,
        const std::string& filepath)
    {
        // ------------------------------------------------------------------
        // 1. Read the existing file so all static data is preserved
        // ------------------------------------------------------------------
        rapidjson::Document doc(rapidjson::kObjectType);

        {
            std::ifstream inFile(filepath);
            if (inFile.is_open())
            {
                std::string content(
                    (std::istreambuf_iterator<char>(inFile)),
                    std::istreambuf_iterator<char>());
                inFile.close();

                if (doc.Parse(content.c_str()).HasParseError())
                    doc.SetObject();
            }
        }

        //rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        // ------------------------------------------------------------------
        // 2. Update player and enemies in place on the original doc,
        //    so the values are ready to be copied into the ordered doc.
        // ------------------------------------------------------------------
        std::string levelKey = "level_" + std::to_string(currentLevel);
        const char* key = levelKey.c_str();

        if (doc.HasMember(key))
        {
            rapidjson::Value& levelObj = doc[key];

            // Player: only x, y, hp - everything else untouched
            if (levelObj.HasMember("player"))
            {
                rapidjson::Value& playerObj = levelObj["player"];
                playerObj["x"] = player.pos.x;
                playerObj["y"] = player.pos.y;
                playerObj["hp"] = player.hp;
            }

            // Enemies: only x, y, hp matched by index
            if (levelObj.HasMember("enemies"))
            {
                rapidjson::Value& enemiesArr = levelObj["enemies"];
                for (rapidjson::SizeType i = 0; i < enemiesArr.Size(); i++)
                {
                    if (i >= static_cast<rapidjson::SizeType>(enemies.size())) break;
                    enemiesArr[i]["x"] = enemies[i].pos.x;
                    enemiesArr[i]["y"] = enemies[i].pos.y;
                    enemiesArr[i]["hp"] = enemies[i].hitPoints;
                }
            }
        }

        // ------------------------------------------------------------------
        // 3. Rebuild a new document in the correct key order.
        // ------------------------------------------------------------------
        rapidjson::Document ordered(rapidjson::kObjectType);
        rapidjson::Document::AllocatorType& oa = ordered.GetAllocator();

        // metadata
        rapidjson::Value metaObj(rapidjson::kObjectType);
        metaObj.AddMember("game_version", rapidjson::Value(metadata.game_version.c_str(), oa), oa);
        metaObj.AddMember("save_date", rapidjson::Value(GetCurrentDateTimeString().c_str(), oa), oa);
        metaObj.AddMember("current_level", metadata.current_level, oa);
        metaObj.AddMember("total_levels", metadata.total_levels, oa);
        metaObj.AddMember("levels_completed", metadata.levels_completed, oa);
        metaObj.AddMember("player_lives", metadata.player_lives, oa);
        ordered.AddMember("metadata", metaObj, oa);

        // level keys in fixed order, deep-copied from the updated doc
        const char* levelOrder[] = { "level_1", "level_2", "level_3", "level_4" };
        for (const char* lvlKey : levelOrder)
        {
            if (doc.HasMember(lvlKey))
            {
                rapidjson::Value copiedLevel(doc[lvlKey], oa);
                ordered.AddMember(rapidjson::Value(lvlKey, oa), copiedLevel, oa);
            }
        }

        // preserve any other top-level keys (e.g. checkpoints)
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            const char* memberName = it->name.GetString();
            if (!ordered.HasMember(memberName))
            {
                rapidjson::Value copiedKey(memberName, oa);
                rapidjson::Value copiedVal(it->value, oa);
                ordered.AddMember(copiedKey, copiedVal, oa);
            }
        }

        // ------------------------------------------------------------------
        // 4. Serialize and write back to file
        // ------------------------------------------------------------------
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 4);
        ordered.Accept(writer);

        std::ofstream outFile(filepath);
        if (!outFile.is_open())
        {
            std::cout << "[SaveGame] ERROR: Could not open file: " << filepath << std::endl;
            return;
        }
        outFile << buffer.GetString();
        outFile.close();
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

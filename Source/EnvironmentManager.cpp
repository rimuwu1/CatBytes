/* Start Header ************************************************************************/
/*!
\file       EnvironmentManager.cpp
\author     Joash ng, joash.ng, 2502780
            Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
            Sim Hui Min, s.huimin, 2503506
\par        joash.ng@digipen.edu
            p.yuxuanlovette@digipen.edu
            s.huimin@digipen.edu
\date       Feb 26 2026
\brief		This file handles all the environment stuff like platforms obstacles and walls.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "EnvironmentManager.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include "LevelIndicator.h"
#include "GameSaveManager.h"
#include "GameStateManager.h"
#include "Player.h"
#include "enemy.h"
#include "EnemyBullet.h"
#include "ObjectManager.h"
#include "Fonts.h"
#include "rapidjson/document.h"
#include <algorithm>
#include <cmath>

// ------------------------------------------------------------------------
// Helper: parse a platform array from a JSON value into a vector
// ------------------------------------------------------------------------
static void ParsePlatforms(const rapidjson::Value& arr, std::vector<Platform>& out)
{
    out.clear();
    for (const auto& p : arr.GetArray()) {
        Platform pf{};
        pf.x = p["x"].GetFloat();
        pf.y = p["y"].GetFloat();
        pf.w = p["width"].GetFloat();
        pf.h = p["height"].GetFloat();
        pf.active = p.HasMember("active") ? p["active"].GetBool() : true;
        out.push_back(pf);
    }
}

// ------------------------------------------------------------------------
// LoadFromConfig  (takes the full document, owns all traversal internally)
// ------------------------------------------------------------------------
void EnvironmentManager::LoadFromConfig(const rapidjson::Document& doc)
{
    // ---- HUD ----
    if (doc.HasMember("ui"))
        m_HUD.InitFromConfig(doc);

    // ---- Button configuration (must be read before creating buttons) ----
    if (doc.HasMember("button") && doc["button"].IsObject()) {
        const auto& btnCfg = doc["button"];
        m_buttonFilePath = btnCfg["file"].GetString();
        m_buttonRows = btnCfg["rows"].GetInt();
        m_buttonCols = btnCfg["cols"].GetInt();
        m_buttonTotalFrames = btnCfg["total_frames"].GetInt();
        m_buttonFrameDuration = btnCfg["frame_duration"].GetFloat();

        m_buttonClips.clear();
        for (const auto& clip : btnCfg["clips"].GetArray()) {
            ButtonClipConfig cfg;
            cfg.name = clip["name"].GetString();
            cfg.start = clip["start"].GetInt();
            cfg.end = clip["end"].GetInt();
            cfg.duration = clip["duration"].GetFloat();
            cfg.loop = clip["loop"].GetBool();
            m_buttonClips.push_back(cfg);
        }
    }
    else {
        // Fallback defaults (optional - prevents crash if config missing)
        m_buttonFilePath = "Assets/Images/buttonSheet2.png";
        m_buttonRows = 3;
        m_buttonCols = 4;
        m_buttonTotalFrames = 9;
        m_buttonFrameDuration = 0.1f;
        m_buttonClips = {
            {"off", 8, 8, 0.1f, false},
            {"transition", 0, 3, 0.2f, false},
            {"on", 4, 7, 0.1f, true}
        };
    }

    // ---- Computer configuration (must be read before creating buttons) ----
    if (doc.HasMember("computer") && doc["computer"].IsObject())
    {
        const auto& compCfg = doc["computer"];
        m_computerFilePath = compCfg["file"].GetString();
        m_computerRows = compCfg["rows"].GetInt();
        m_computerCols = compCfg["columns"].GetInt();
        m_computerTotalFrames = compCfg["total_frames"].GetInt();
        m_computerFrameDuration = compCfg["frame_duration"].GetFloat();

        m_computerClips.clear();
        
        for (const auto& clip : compCfg["clips"].GetArray())
        {
            ButtonClipConfig cfg;
            cfg.name = clip["name"].GetString();
            cfg.start = clip["start"].GetInt();
            cfg.end = clip["end"].GetInt();
            cfg.duration = clip["duration"].GetFloat();
            cfg.loop = clip["loop"].GetBool();
            m_computerClips.push_back(cfg);
        }
    }
    else 
    {
        m_computerFilePath = "Assets/Images/computer.png";
        m_computerRows = 2;
        m_computerCols = 4;
        m_computerTotalFrames = 8;
        m_computerFrameDuration = 0.1f;
        m_computerClips = {
            { "off", 0, 0, 0.1f, false },
            { "transition", 0, 3, 0.5f, false },
            { "on", 4, 7, 0.1f, true }
        };
    }

    // ---- Laser Indicators configuration ----
    if (doc.HasMember("laser_indicators") && doc["laser_indicators"].IsObject())
    {
        const auto& lsInd = doc["laser_indicators"];

        // ---- Horizontal Indicators ----
        if (lsInd.HasMember("hLeft") && lsInd["hLeft"].IsObject())
        {
            const auto& leftInd = lsInd["hLeft"];
            m_leftIndicatorFilePath = leftInd["file"].GetString();
            m_hIndicatorRows = leftInd["rows"].GetInt();
            m_hIndicatorCols = leftInd["cols"].GetInt();
            m_hIndicatorTotalFrames = leftInd["total_frames"].GetInt();
            m_hIndicatorFrameDuration = leftInd["frame_duration"].GetFloat();

            m_hIndicatorClips.clear();

            for (const auto& clip : leftInd["clips"].GetArray())
            {
                ButtonClipConfig cfg;
                cfg.name = clip["name"].GetString();
                cfg.start = clip["start"].GetInt();
                cfg.end = clip["end"].GetInt();
                cfg.duration = clip["duration"].GetFloat();
                cfg.loop = clip["loop"].GetBool();

                m_hIndicatorClips.push_back(cfg);
            }
        }

        if (lsInd.HasMember("hRight") && lsInd["hRight"].IsObject())
        {
            const auto& rightInd = lsInd["hRight"];
            m_rightIndicatorFilePath = rightInd["file"].GetString();
        }

        if (lsInd.HasMember("vert") && lsInd["vert"].IsObject())
        {
            const auto& vertInd = lsInd["vert"];
            m_verticalIndicatorFilePath = vertInd["file"].GetString();
            m_vIndicatorRows = vertInd["rows"].GetInt();
            m_vIndicatorCols = vertInd["cols"].GetInt();
            m_vIndicatorTotalFrames = vertInd["total_frames"].GetInt();
            m_vIndicatorFrameDuration = vertInd["frame_duration"].GetFloat();

            m_vIndicatorClips.clear();

            for (const auto& clip : vertInd["clips"].GetArray())
            {
                ButtonClipConfig cfg;
                cfg.name = clip["name"].GetString();
                cfg.start = clip["start"].GetInt();
                cfg.end = clip["end"].GetInt();
                cfg.duration = clip["duration"].GetFloat();
                cfg.loop = clip["loop"].GetBool();

                m_vIndicatorClips.push_back(cfg);
            }

        }

    }

    // ---- Platforms per level ----
    const struct { const char* key; std::vector<Platform>* target; } platformMaps[] = {
        { "level_1", &m_level1Platforms },
        { "level_2", &m_level2Platforms },
        { "level_3", &m_level3Platforms },
        { "level_4", &m_bossPlatforms   },
    };
    for (const auto& entry : platformMaps) {
        if (doc.HasMember(entry.key) && doc[entry.key].HasMember("platforms"))
            ParsePlatforms(doc[entry.key]["platforms"], *entry.target);
    }

    // ---- Walls (level_1 only) ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("walls")) {
        m_wallPlatforms.clear();
        for (const auto& w : doc["level_1"]["walls"].GetArray()) {
            Platform wall{};
            wall.x = w["x"].GetFloat();
            wall.y = w["y"].GetFloat();
            wall.w = w["width"].GetFloat();
            wall.h = w["height"].GetFloat();
            wall.active = true;   // walls are always active
            m_wallPlatforms.push_back(wall);
        }
    }

    // ---- Level 1 buttons ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("buttons")) {
        m_level1Buttons.clear();
        for (const auto& b : doc["level_1"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;
            btn.btnPrompt = b.HasMember("prompt") ? b["prompt"].GetString() : "Press E to toggle platforms";

            // --- Create per-button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            m_level1Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_1 only) ----
    if (doc.HasMember("level_1") && doc["level_1"].HasMember("obstacles")) {
        m_level1Obstacles.clear();
        for (const auto& o : doc["level_1"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true;
            
             // If this is a spike obstacle, instantiate per-obstacle SpriteSheet
             if (obs.isSpike) {
                 obs.sprite = std::make_unique<SpriteSheet>(
                     m_spikeFilePath.c_str(),
                     m_spikeRows,
                     m_spikeCols,
                     m_spikeTotalFrames,
                     m_spikeFrameDuration
                 );
                 
                 for (const auto& clipCfg : m_spikeClips) {
                     obs.sprite->AddClip(
                         clipCfg.name.c_str(),
                         clipCfg.start,
                         clipCfg.end,
                         clipCfg.duration,
                         clipCfg.loop
                     );
                 }
                 
                 obs.sprite->Play("on");
                 obs.active = true;
                 obs.prevActive = true;
                 obs.spriteInitialized = true;
             }
             
             m_level1Obstacles.push_back(std::move(obs));
        }
    }

    // ---- Level 2 buttons ----
    if (doc.HasMember("level_2") && doc["level_2"].HasMember("buttons")) {
        m_level2Buttons.clear();
        for (const auto& b : doc["level_2"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;
            btn.btnPrompt = b.HasMember("prompt") ? b["prompt"].GetString() : "Press E to toggle platforms";

            // --- Create per?button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            m_level2Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_2 only) ----
    if (doc.HasMember("level_2") && doc["level_2"].HasMember("obstacles")) {
        m_level2Obstacles.clear();
        for (const auto& o : doc["level_2"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true;
            
             // If this is a spike obstacle, instantiate per-obstacle SpriteSheet
             if (obs.isSpike) {
                 obs.sprite = std::make_unique<SpriteSheet>(
                     m_spikeFilePath.c_str(),
                     m_spikeRows,
                     m_spikeCols,
                     m_spikeTotalFrames,
                     m_spikeFrameDuration
                 );
                 
                 for (const auto& clipCfg : m_spikeClips) {
                     obs.sprite->AddClip(
                         clipCfg.name.c_str(),
                         clipCfg.start,
                         clipCfg.end,
                         clipCfg.duration,
                         clipCfg.loop
                     );
                 }
                 
                 obs.sprite->Play("on");
                 obs.active = true;
                 obs.prevActive = true;
                 obs.spriteInitialized = true;
             }
             
             m_level2Obstacles.push_back(std::move(obs));
        }
    }
 
    // ---- Lasers (level_2 only) ----
    if (doc.HasMember("level_2") && doc["level_2"].HasMember("lasers")) {
        m_level2Lasers.clear();
        for (const auto& l : doc["level_2"]["lasers"].GetArray()) {
            PlatformLaser ls{};
            ls.x1 = l["x1"].GetFloat();
            ls.x2 = l["x2"].GetFloat();
            ls.y1 = l["y1"].GetFloat();
            ls.y2 = l["y2"].GetFloat();
            ls.w = l["width"].GetFloat();
            ls.laserActive = l.HasMember("active") ? l["active"].GetBool() : true;
            ls.laserToggle = l.HasMember("toggeable") ? l["toggeable"].GetBool() : false;
            m_level2Lasers.push_back(ls);
        }
    }

    // ---- Walls (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("walls")) {
        m_level3WallPlatforms.clear();
        for (const auto& w : doc["level_3"]["walls"].GetArray()) {
            Platform wall{};
            wall.x = w["x"].GetFloat();
            wall.y = w["y"].GetFloat();
            wall.w = w["width"].GetFloat();
            wall.h = w["height"].GetFloat();
            wall.active = true;   // walls are always active
            m_level3WallPlatforms.push_back(wall);
        }
    }

    // ---- Level 3 buttons ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("buttons")) {
        m_level3Buttons.clear();
        for (const auto& b : doc["level_3"]["buttons"].GetArray()) {
            PlatformButton btn{};
            btn.x = b["x"].GetFloat();
            btn.y = b["y"].GetFloat();
            btn.w = b["width"].GetFloat();
            btn.h = b["height"].GetFloat();
            btn.wasPressed = false;
            btn.btnPrompt = b.HasMember("prompt") ? b["prompt"].GetString() : "Press E to toggle platforms";

            // --- Create per?button sprite from stored config ---
            btn.buttonSprite = std::make_unique<SpriteSheet>(
                m_buttonFilePath.c_str(),
                m_buttonRows,
                m_buttonCols,
                m_buttonTotalFrames,
                m_buttonFrameDuration
            );

            for (const auto& clipCfg : m_buttonClips) {
                btn.buttonSprite->AddClip(
                    clipCfg.name.c_str(),
                    clipCfg.start,
                    clipCfg.end,
                    clipCfg.duration,
                    clipCfg.loop
                );
            }
            btn.buttonSprite->Play("off");   // initial default

            if (b.HasMember("platformIndices") && b["platformIndices"].IsArray()) {
                for (const auto& index : b["platformIndices"].GetArray()) {
                    btn.platformIndices.push_back(index.GetInt());
                }
            }

            if (b.HasMember("wallIndices") && b["wallIndices"].IsArray()) {
                for (const auto& index : b["wallIndices"].GetArray()) {
                    btn.wallIndices.push_back(index.GetInt());
                }
            }

            m_level3Buttons.push_back(std::move(btn));
        }
    }

    // ---- Obstacles (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("obstacles")) {
        m_level3Obstacles.clear();
        for (const auto& o : doc["level_3"]["obstacles"].GetArray()) {
            PlatformObstacle obs{};
            obs.x = o["x"].GetFloat();
            obs.y = o["y"].GetFloat();
            obs.w = o["width"].GetFloat();
            obs.h = o["height"].GetFloat();
            obs.r = o.HasMember("rotation") ? o["rotation"].GetFloat() : 0.0f;
            obs.isSpike = o.HasMember("is_spike") ? o["is_spike"].GetBool() : true;
            
             // If this is a spike obstacle, instantiate per-obstacle SpriteSheet
             if (obs.isSpike) {
                 obs.sprite = std::make_unique<SpriteSheet>(
                     m_spikeFilePath.c_str(),
                     m_spikeRows,
                     m_spikeCols,
                     m_spikeTotalFrames,
                     m_spikeFrameDuration
                 );
                 
                 for (const auto& clipCfg : m_spikeClips) {
                     obs.sprite->AddClip(
                         clipCfg.name.c_str(),
                         clipCfg.start,
                         clipCfg.end,
                         clipCfg.duration,
                         clipCfg.loop
                     );
                 }
                 
                 obs.sprite->Play("on");
                 obs.active = true;
                 obs.prevActive = true;
                 obs.spriteInitialized = true;
             }
             
             m_level3Obstacles.push_back(std::move(obs));
        }
    }

    // ---- Lasers (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("lasers")) {
        m_level3Lasers.clear();
        for (const auto& l : doc["level_3"]["lasers"].GetArray()) {
            PlatformLaser ls{};
            ls.x1 = l["x1"].GetFloat();
            ls.x2 = l["x2"].GetFloat();
            ls.y1 = l["y1"].GetFloat();
            ls.y2 = l["y2"].GetFloat();
            ls.w = l["width"].GetFloat();
            ls.laserActive = l.HasMember("active") ? l["active"].GetBool() : true;
            ls.laserToggle = l.HasMember("toggeable") ? l["toggeable"].GetBool() : false;
            m_level3Lasers.push_back(ls);
        }
    }

    // ---- Computers (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("computers")) {
        m_level3Computers.clear();
        for (const auto& c : doc["level_3"]["computers"].GetArray()) {
            PlatformComputer comp{};
            comp.x = c["x"].GetFloat();
            comp.y = c["y"].GetFloat();
            comp.w = c["width"].GetFloat();
            comp.h = c["height"].GetFloat();

            comp.computerSprite = std::make_unique<SpriteSheet>(
                m_computerFilePath.c_str(),
                m_computerRows,
                m_computerCols,
                m_computerTotalFrames,
                m_computerFrameDuration
            );

            for (const auto& clipCfg : m_computerClips)
            {
                comp.computerSprite->AddClip(clipCfg.name.c_str(), clipCfg.start,
                    clipCfg.end, clipCfg.duration, clipCfg.loop);
            }
            comp.computerSprite->Play("off");

            // ---- direction + indicator sprites ----
            bool isHorizontal = true;
            if (c.HasMember("direction"))
            {
                isHorizontal = (std::string(c["direction"].GetString()) == "horizontal");
            }

            comp.direction = isHorizontal ? BeamDirection::Horizontal : BeamDirection::Vertical;
            comp.beamX1 = c["beamX1"].GetFloat();
            comp.beamX2 = c["beamX2"].GetFloat();
            comp.beamY1 = c["beamY1"].GetFloat();
            comp.beamY2 = c["beamY2"].GetFloat();
            comp.beamStartX = c["beamStartX"].GetFloat();
            comp.beamEndX = c["beamEndX"].GetFloat();
            comp.beamStartY = c["beamStartY"].GetFloat();
            comp.beamEndY = c["beamEndY"].GetFloat();
            comp.beamW = c.HasMember("beamWidth") ? c["beamWidth"].GetFloat() : 50.0f;

            comp.indW = c.HasMember("indWidth") ? c["indWidth"].GetFloat() : comp.w;
            comp.indH = c.HasMember("indHeight") ? c["indHeight"].GetFloat() : comp.h;

            if (isHorizontal)
            {
                comp.indicatorLeft = std::make_unique<SpriteSheet>(
                    m_leftIndicatorFilePath.c_str(),
                    m_hIndicatorRows, m_hIndicatorCols,
                    m_hIndicatorTotalFrames, m_hIndicatorFrameDuration
                );

                for (const auto& clipCfg : m_hIndicatorClips)
                {
                    comp.indicatorLeft->AddClip(clipCfg.name.c_str(), clipCfg.start, clipCfg.end,
                        clipCfg.duration, clipCfg.loop);
                }
                comp.indicatorLeft->Play("off");

                comp.indicatorRight = std::make_unique<SpriteSheet>(
                    m_rightIndicatorFilePath.c_str(),
                    m_hIndicatorRows, m_hIndicatorCols,
                    m_hIndicatorTotalFrames, m_hIndicatorFrameDuration
                );

                for (const auto& clipCfg : m_hIndicatorClips)
                {
                    comp.indicatorRight->AddClip(clipCfg.name.c_str(), clipCfg.start, clipCfg.end,
                        clipCfg.duration, clipCfg.loop);
                }
                comp.indicatorRight->Play("off");
            }
            else 
            {
                comp.indicatorLeft = std::make_unique<SpriteSheet>(
                    m_verticalIndicatorFilePath.c_str(),
                    m_vIndicatorRows, m_vIndicatorCols,
                    m_vIndicatorTotalFrames, m_vIndicatorFrameDuration
                );

                for (const auto& clipCfg : m_vIndicatorClips)
                {
                    comp.indicatorLeft->AddClip(clipCfg.name.c_str(), clipCfg.start, clipCfg.end,
                        clipCfg.duration, clipCfg.loop);
                }
                comp.indicatorLeft->Play("off");

                comp.indicatorRight = nullptr;
            }

            if (c.HasMember("laserIndices") && c["laserIndices"].IsArray())
            {
                for (const auto& index : c["laserIndices"].GetArray())
                {
                    comp.laserIndices.push_back(index.GetInt());
                }
            }
            m_level3Computers.push_back(std::move(comp));
        }
    }

    // ---- Toggleable Walls (level_3 only) ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("toggleable_walls")) {
        m_level3ToggleWalls.clear();
        for (const auto& tw : doc["level_3"]["toggleable_walls"].GetArray()) {
            Platform toggleWall{};
            toggleWall.x = tw["x"].GetFloat();
            toggleWall.y = tw["y"].GetFloat();
            toggleWall.w = tw["width"].GetFloat();
            toggleWall.h = tw["height"].GetFloat();
            toggleWall.active = tw.HasMember("active") ? tw["active"].GetBool() : true;
            
            m_level3ToggleWalls.push_back(toggleWall);
        }
    }

    // ---- boss door ----
    if (doc.HasMember("level_3") && doc["level_3"].HasMember("boss_door"))
    {
        const auto& bd = doc["level_3"]["boss_door"];
        m_bossDoor.x             = bd["x"].GetFloat();
        m_bossDoor.y             = bd["y"].GetFloat();
        m_bossDoor.w             = bd["width"].GetFloat();
        m_bossDoor.h             = bd["height"].GetFloat();
        m_bossDoor.liftX         = bd["lift_x"].GetFloat();
        m_bossDoor.liftY         = bd["lift_y"].GetFloat();
        m_bossDoor.liftW         = bd.HasMember("lift_width")  ? bd["lift_width"].GetFloat()  : 340.0f;
        m_bossDoor.liftH         = bd.HasMember("lift_height") ? bd["lift_height"].GetFloat() : 3600.0f;
        m_bossDoor.triggerRadius = bd["trigger_radius"].GetFloat();
        m_bossDoor.prompt        = bd["prompt"].GetString();
        m_bossDoorLoaded         = true;
    }

    // ---- Checkpoints ----
    if (doc.HasMember("checkpoints")) {
        m_checkpoints.clear();
        for (const auto& pt : doc["checkpoints"].GetArray()) {
            Checkpoint cp{};
            cp.x = pt["x"].GetFloat();
            cp.y = pt["y"].GetFloat();
            cp.w = pt["width"].GetFloat();
            cp.h = pt["height"].GetFloat();
            m_checkpoints.push_back(cp);
        }
    }

    // Reset runtime state so a reload/restart starts clean
    m_checkpointSaved = false;
    m_saveRequested = false;
    // Mark static cache dirty so the new configuration is rebuilt on next Draw()
    MarkStaticDirty();
}

// ------------------------------------------------------------------------
// Load environment assets from config (textures and spritesheets)
// ------------------------------------------------------------------------
void EnvironmentManager::LoadAssetsFromConfig(const rapidjson::Document& doc)
{
    if (!doc.HasMember("environment") || !doc["environment"].IsObject()) return;
    const auto& env = doc["environment"];

    auto loadTex = [&](const char* key) -> AEGfxTexture* {
        if (env.HasMember(key) && env[key].IsString())
            return TextureManager::Get().LoadTexture(env[key].GetString());
        return nullptr;
    };

    m_leftTex   = loadTex("platform_left");
    m_midTex    = loadTex("platform_mid");
    m_rightTex  = loadTex("platform_right");
    m_laserTex  = loadTex("laser");
    m_wallLeftTex  = loadTex("wall_left");
    m_wallMidTex   = loadTex("wall_mid");
    m_wallRightTex = loadTex("wall_right");

    // Load parallax layers from config
    if (env.HasMember("parallax") && env["parallax"].IsObject()) {
        const auto& parallax = env["parallax"];
        const char* layerNames[3] = {"back", "middle", "front"};
        float speeds[3] = {0.3f, 0.6f, 1.0f};
        
        for (int i = 0; i < 3; i++) {
            if (parallax.HasMember(layerNames[i]) && parallax[layerNames[i]].IsString()) {
                m_parallaxLayers[i].texture = TextureManager::Get().LoadTexture(parallax[layerNames[i]].GetString());
                m_parallaxLayers[i].speed = speeds[i];
            }
        }
    }

    if (env.HasMember("hover_anim") && env["hover_anim"].IsObject()) {
        const auto& a = env["hover_anim"];
        m_hoverAnim = std::make_unique<SpriteSheet>(
            a["file"].GetString(),
            a["rows"].GetInt(),
            a["cols"].GetInt(),
            a["start"].GetInt(),
            static_cast<float>(a["duration"].GetDouble())
        );
    }

    if (env.HasMember("checkpoint_anim") && env["checkpoint_anim"].IsObject()) {
        const auto& a = env["checkpoint_anim"];
        m_checkpointAnim = std::make_unique<SpriteSheet>(
            a["file"].GetString(),
            a["rows"].GetInt(),
            a["cols"].GetInt(),
            a["start"].GetInt(),
            static_cast<float>(a["duration"].GetDouble())
        );
    }

    // ---- Spike obstacle configuration ----
    if (env.HasMember("spike_anim") && env["spike_anim"].IsObject()) {
        const auto& spikeConfig = env["spike_anim"];
        
        // Read sprite sheet metadata
        m_spikeFilePath = spikeConfig.HasMember("file") ? spikeConfig["file"].GetString() : "Assets/Images/spikeObstacle.png";
        m_spikeRows = spikeConfig.HasMember("rows") ? spikeConfig["rows"].GetInt() : 1;
        m_spikeCols = spikeConfig.HasMember("cols") ? spikeConfig["cols"].GetInt() : 10;
        m_spikeTotalFrames = spikeConfig.HasMember("total_frames") ? spikeConfig["total_frames"].GetInt() : 10;
        m_spikeFrameDuration = spikeConfig.HasMember("frame_duration") ? spikeConfig["frame_duration"].GetFloat() : 0.05f;

        // Create sprite sheet with loaded metadata
         // No shared spike animation; each obstacle owns its own sprite

        // Parse clip configs if present; apply defaults per field for robustness
        if (spikeConfig.HasMember("clips") && spikeConfig["clips"].IsArray()) {
            std::vector<ButtonClipConfig> tempClips;
            for (const auto& clip : spikeConfig["clips"].GetArray()) {
                ButtonClipConfig cfg;
                
                // name: required, but guard with HasMember and isString
                if (clip.HasMember("name") && clip["name"].IsString()) {
                    cfg.name = clip["name"].GetString();
                } else {
                    // Skip clips without a name (or apply a default name if desired)
                    continue;
                }
                
                // start: int, default 0
                cfg.start = (clip.HasMember("start") && clip["start"].IsInt()) ? clip["start"].GetInt() : 0;
                
                // end: int, default 0
                cfg.end = (clip.HasMember("end") && clip["end"].IsInt()) ? clip["end"].GetInt() : 0;
                
                // duration: float, default m_spikeFrameDuration
                cfg.duration = (clip.HasMember("duration") && clip["duration"].IsNumber()) ? 
                    static_cast<float>(clip["duration"].GetDouble()) : m_spikeFrameDuration;
                
                // loop: bool, default false
                cfg.loop = (clip.HasMember("loop") && clip["loop"].IsBool()) ? clip["loop"].GetBool() : false;
                
                tempClips.push_back(cfg);
            }
            // Only replace m_spikeClips if at least one valid clip was parsed
            if (!tempClips.empty()) {
                m_spikeClips = std::move(tempClips);
            }
            // Else: clips array was present but empty or all entries were skipped (invalid).
            // m_spikeClips retains its default from header initialization.
        }
        // If clips array is absent, m_spikeClips retains its default from header initialization
    }
    else {
        // Fallback defaults if spike_anim section missing entirely
        m_spikeFilePath = "Assets/Images/spikeObstacle.png";
        m_spikeRows = 1;
        m_spikeCols = 10;
        m_spikeTotalFrames = 10;
        m_spikeFrameDuration = 0.05f;
        m_spikeClips = {
            { "on", 9, 9, 0.05f, true },
            { "toggle", 1, 8, 0.05f, false },
            { "off", 0, 0, 0.05f, true }
        };

        // No shared spike animation; each obstacle owns its own sprite
    }

    // boss door textures
    if (m_bossDoorLoaded && doc.HasMember("level_3") && doc["level_3"].HasMember("boss_door"))
    {
        const auto& bd = doc["level_3"]["boss_door"];
        m_bossDoor.doorTex = TextureManager::Get().LoadTexture(bd["file"].GetString());
        m_bossDoor.liftAnim = std::make_unique<SpriteSheet>(
            bd["lift_file"].GetString(),
            bd["lift_rows"].GetInt(),
            bd["lift_cols"].GetInt()
        );
        m_bossDoor.liftAnim->AddClip("rise", 0, 16, 0.08f, false);
        m_bossDoor.liftAnim->SetFrame(0);
    }
}

// ------------------------------------------------------------------------
// Initialize textures and one-time setup only, safe to call once at startup
// ------------------------------------------------------------------------
void EnvironmentManager::Initialize()
{
    m_currentColour      = m_backgroundColours[0];
    m_previousSelection  = -1;
    // Assets loaded via LoadAssetsFromConfig — called from ApplyConfigToManagers in MainGame.cpp
    LevelIndicator_Initialize();
}

// ------------------------------------------------------------------------
void EnvironmentManager::Update(float dt, Player& player, float cameraY)
{
    m_HUD.Update(dt, player, player.weapon);

    if (m_hoverAnim) m_hoverAnim->Update(dt);
    if (m_checkpointAnim) m_checkpointAnim->Update(dt);

    UpdateBackground(cameraY);
    
    // Update parallax offset based on camera Y
    m_parallaxY = cameraY;

    int section = GetSectionFromY(cameraY);
    if (section != m_previousSelection) {
        LevelIndicator_Show(section);
        m_previousSelection = section;
    }
    LevelIndicator_Update(dt);

     // update spike timer
     auto updateObsTimer = [&](std::vector<PlatformObstacle>& obstacles)
         {
             for (auto& o : obstacles)
             {
                 if (!o.isSpike) continue;
                 o.timer += dt;
                 if (o.timer >= o.spikeInterval)
                 {
                     o.timer = 0.0f;
                     o.active = !o.active;
                 }
             }
         };
     updateObsTimer(m_level1Obstacles);
     updateObsTimer(m_level2Obstacles);
     updateObsTimer(m_level3Obstacles);

      // Per-obstacle spike animation state machine
       auto updateSpikeAnimations = [&](std::vector<PlatformObstacle>& obstacles)
           {
               for (auto& o : obstacles)
               {
                   if (!o.isSpike || !o.sprite) continue;

                   // Initialize sprite if not yet done
                   if (!o.spriteInitialized) {
                       o.sprite->Play("on");
                       o.prevActive = true;
                       o.spriteInitialized = true;
                   }

                   // Detect state changes and trigger transitions
                   // Off -> On: play toggle animation then switch to "on"
                   // On -> Off: immediately play "off" (no toggle)
                   if (o.prevActive != o.active) {
                       if (o.active) {
                           // Transitioning OFF -> ON: play toggle
                           o.sprite->Play("toggle");
                       } else {
                           // Transitioning ON -> OFF: immediately show off
                           o.sprite->Play("off");
                       }
                   }

                   // Update sprite animation
                   o.sprite->Update(dt);

                   // If toggle animation finished, switch to "on" clip
                   if (o.sprite->GetCurrentClip() == "toggle" && !o.sprite->IsPlaying()) {
                       o.sprite->Play("on");
                   }

                   // Update previous state
                   o.prevActive = o.active;
              }
          };
     updateSpikeAnimations(m_level1Obstacles);
     updateSpikeAnimations(m_level2Obstacles);
     updateSpikeAnimations(m_level3Obstacles);

    // boss door proxmity check
    if (m_bossDoorLoaded && !m_liftSeq.active)
    {
        float dx = player.pos.x - m_bossDoor.x;
        float dy = player.pos.y - m_bossDoor.y;
        float dist = sqrtf(dx * dx + dy * dy);
        m_bossDoor.playerNear = (dist < m_bossDoor.triggerRadius);

        if (m_bossDoor.playerNear && AEInputCheckTriggered('E') && !m_bossDoor.activated)
        {
            // Store original position for save, then hide player for animation
            m_bossDoor.savedPlayerX = player.pos.x;
            m_bossDoor.savedPlayerY = player.pos.y;
            
            m_bossDoor.activated = true;
            m_liftSeq.active = true;
            m_liftSeq.liftPosY = m_bossDoor.liftY;
            m_liftSeq.camY = BossLiftSequence::CAM_START_Y;
            m_liftSeq.fadeAlpha = 0.0f;
            m_liftSeq.fadingIn = false;
            if (m_bossDoor.liftAnim)
                m_bossDoor.liftAnim->Play("rise");

            // Hide player off screen during lift sequence
            player.pos.x = -1000.0f;
            player.pos.y = 7400.0f;
            
            // Trigger save AFTER storing position but while lift is active
            RequestSave();
        }
    }

    // lift sequence tick
    if (m_liftSeq.active)
    {
        m_liftSeq.camY += BossLiftSequence::LIFT_SPEED * dt;

        if (m_bossDoor.liftAnim)
            m_bossDoor.liftAnim->Update(dt);

        if (m_liftSeq.camY >= BossLiftSequence::FADE_START)
            m_liftSeq.fadingIn = true;

        if (m_liftSeq.fadingIn)
        {
            m_liftSeq.fadeAlpha += BossLiftSequence::FADE_SPEED * dt;
            if (m_liftSeq.fadeAlpha > 1.0f) m_liftSeq.fadeAlpha = 1.0f;
        }

        if (m_liftSeq.fadeAlpha >= 1.0f)
        {
            m_liftSeq.active = false;
            m_bossDoor.activated = false;
            m_liftSeq.fadeAlpha = 0.0f;
            m_liftSeq.fadingIn = false;
            if (m_bossDoor.liftAnim)
                m_bossDoor.liftAnim->SetFrame(0);
            GameStateManager::Get().next = GS_BOSSROOM;
        }
    }

    // update laser timer
    auto updateLaserTimer = [&](std::vector<PlatformLaser>& lasers)
        {
            for (auto& l : lasers)
            {
                l.timer += dt;
                if (l.timer >= l.laserInterval)
                {
                    l.timer = 0.0f;
                    l.laserActive = !l.laserActive;
                }
            }
        };
    updateLaserTimer(m_level2Lasers);
    updateLaserTimer(m_level3Lasers);

    // laser beam delay
    for (auto& comp : m_level3Computers)
    {
        // pre-activation delay
        if (comp.pendingActivate)
        {
            comp.preActivateDelay -= dt;
            if (comp.preActivateDelay <= 0.0f)
            {
                comp.pendingActivate = false;
                comp.beamActive = true;
                comp.delayBeamActivate = 1.5f;
                comp.indicatorsVisible = true;
                comp.beamVisible = false;
            }
        }

        // delay beam visibility
        if (comp.beamActive && !comp.beamVisible && comp.delayBeamActivate > 0.0f)
        {
            comp.delayBeamActivate -= dt;
            if (comp.delayBeamActivate <= 0.0f)
            {
                comp.beamVisible = true;
                comp.nowShowBeam = true;
                comp.beamKilled = false;
                comp.beamDuration = 1.0f;
            }
        }

        // reset when turned off
        if (!comp.beamActive && !comp.pendingActivate)
        {
            comp.beamVisible = false;
            comp.indicatorsVisible = false;
            comp.delayBeamActivate = 0.0f;
            comp.nowShowBeam = false;
        }
    }
}

void EnvironmentManager::LoadBossArenaFromConfig(const rapidjson::Document& doc)
{
    if (!doc.HasMember("boss_arena")) return;
    const auto& arena = doc["boss_arena"];

    // walls
    if (arena.HasMember("walls"))
    {
        m_wallPlatforms.clear();
        for (const auto& w : arena["walls"].GetArray()) {
            Platform wall{};
            wall.x = w["x"].GetFloat(); wall.y = w["y"].GetFloat();
            wall.w = w["width"].GetFloat(); wall.h = w["height"].GetFloat();
            wall.active = true;
            m_wallPlatforms.push_back(wall);
        }
    }

    // platforms
    if (arena.HasMember("platforms"))
        ParsePlatforms(arena["platforms"], m_bossPlatforms);

    MarkStaticDirty();
}

// ------------------------------------------------------------------------
// Static batch cache helpers
// ------------------------------------------------------------------------
void EnvironmentManager::RebuildStaticCache()
{
    m_staticCache.clear();

    auto addSprite = [&](AEGfxTexture* tex, float uvW, float uvH,
        float x, float y, float w, float h,
        float uvOffX, float uvOffY,
        float opacity = 1.0f, float rotation = 0.0f) {
            m_staticCache.push_back({ tex, uvW, uvH, x, y, w, h, uvOffX, uvOffY, opacity, rotation });
        };

    const float capWidth = 32.0f;

    // Collect platforms (no culling here)
    auto collectPlatforms = [&](const std::vector<Platform>& platforms) {
        for (const auto& p : platforms) {
            if (!p.active) continue;

            float leftX = p.x - p.w * 0.5f + capWidth * 0.5f;
            float rightX = p.x + p.w * 0.5f - capWidth * 0.5f;

            // Left cap
            addSprite(m_leftTex, 1.0f, 1.0f, leftX, p.y, capWidth, p.h, 0.0f, 0.0f);
            // Right cap
            addSprite(m_rightTex, 1.0f, 1.0f, rightX, p.y, capWidth, p.h, 0.0f, 0.0f);

            float midStartX = leftX + capWidth * 0.5f;
            float midEndX = rightX - capWidth * 0.5f;
            float midWidth = midEndX - midStartX;
            if (midWidth > 0.0f) {
                float midCenterX = (midStartX + midEndX) * 0.5f;
                // Middle section
                addSprite(m_midTex, 1.0f, 1.0f, midCenterX, p.y, midWidth, p.h, 0.0f, 0.0f);
                // Note: hover animation is dynamic (animated UV offsets) and is
                // drawn per-frame in Draw(). Do NOT cache hover sprites here.
            }
        }
    };

    collectPlatforms(m_level1Platforms);
    collectPlatforms(m_level2Platforms);
    collectPlatforms(m_level3Platforms);
    collectPlatforms(m_bossPlatforms);

    // collect walls
    auto collectWalls = [&](const std::vector<Platform>& walls) {
        for (const auto& w : walls)
        {
            if (!w.active) continue;

            float leftX = w.x - w.w * 0.5f + capWidth * 0.5f;
            float rightX = w.x + w.w * 0.5f - capWidth * 0.5f;

            addSprite(m_wallLeftTex, 1.0f, 1.0f, leftX, w.y, capWidth, w.h, 0.0f, 0.0f);
            addSprite(m_wallRightTex, 1.0f, 1.0f, rightX, w.y, capWidth, w.h, 0.0f, 0.0f);

            float midStartX = leftX + capWidth * 0.5f;
            float midEndX = rightX - capWidth * 0.5f;
            float midWidth = midEndX - midStartX;

            if (midWidth > 0.0f)
            {
                float midCenterX = (midStartX + midEndX) * 0.5f;
                addSprite(m_wallMidTex, 1.0f, 1.0f, midCenterX, w.y, midWidth, w.h, 0.0f, 0.0f);
            }
        }
    };

    collectWalls(m_wallPlatforms);
    collectWalls(m_level3WallPlatforms);
    collectWalls(m_level3ToggleWalls);

    // NOTE: spikes are animated and must not be added to the static cache

    // NOTE: Checkpoints and hover animations are dynamic and must be
    // submitted per-frame in Draw(); do not cache checkpoint sprites here.

    // Sort by texture pointer
    std::sort(m_staticCache.begin(), m_staticCache.end(),
        [](const QueuedSprite& a, const QueuedSprite& b) {
            return a.texture < b.texture;
        });

    m_staticBatchDirty = false;
}

void EnvironmentManager::FlushStaticCache(float camY, float cullHalf)
{
    if (m_staticCache.empty()) return;

    MeshManager& mm = MeshManager::Get();

    size_t i = 0;
    while (i < m_staticCache.size()) {
        const QueuedSprite& first = m_staticCache[i];
        mm.BeginBatch(first.texture, first.uvW, first.uvH);
        do {
            const QueuedSprite& s = m_staticCache[i];
            SpriteBatchItem item;
            item.x = s.x;
            item.y = s.y;
            item.width = s.w;
            item.height = s.h;
            item.uvOffsetX = s.uvOffX;
            item.uvOffsetY = s.uvOffY;
            item.opacity = s.opacity;
            item.rotation = s.rotation;
            bool visible = (s.y + s.h * 0.5f) >= (camY - cullHalf) &&
                           (s.y - s.h * 0.5f) <= (camY + cullHalf);
            if (visible)
                mm.QueueSprite(item);
            ++i; // always advance the index to preserve grouping logic
        } while (i < m_staticCache.size() &&
            m_staticCache[i].texture == first.texture &&
            m_staticCache[i].uvW == first.uvW &&
            m_staticCache[i].uvH == first.uvH);
        mm.EndBatch();
    }
}

void EnvironmentManager::MarkStaticDirty()
{
    m_staticBatchDirty = true;
}

// ------------------------------------------------------------------------
void EnvironmentManager::DrawWorld(float camX, float camY, PlayerWeapon weapon, const Player& player, float screenHalfH)
{
    MeshManager& mm = MeshManager::Get();
    UNREFERENCED_PARAMETER(weapon);

    const float CULL_MARGIN = 200.0f;
    const float cullHalf = screenHalfH + CULL_MARGIN;

    auto inView = [&](float objY, float halfH) -> bool {
        return (objY + halfH) >= (camY - cullHalf) &&
            (objY - halfH) <= (camY + cullHalf);
        };

    // --------------------------------------------------------------------
    // 1. Static geometry (platforms, obstacles, checkpoints)
    // --------------------------------------------------------------------
    if (m_staticBatchDirty)
        RebuildStaticCache();
    FlushStaticCache(camY, cullHalf);

    // --------------------------------------------------------------------
    // 1.b Dynamic animated sprites (hover animation & checkpoints)
    // These use per-frame UV offsets so they must be submitted each frame.
    // --------------------------------------------------------------------
    // Check if there are any dynamic sprites to draw (hover, checkpoint, or spike obstacles)
    auto hasObstaclesWithSprites = [](const std::vector<PlatformObstacle>& obs) {
        for (const auto& o : obs) {
            if (o.isSpike && o.sprite) return true;
        }
        return false;
    };
    bool hasSprites = m_hoverAnim || m_checkpointAnim ||
        hasObstaclesWithSprites(m_level1Obstacles) ||
        hasObstaclesWithSprites(m_level2Obstacles) ||
        hasObstaclesWithSprites(m_level3Obstacles);

    if (hasSprites) {
        // Build a temporary batch for animated sprites
        m_spriteBatch.clear();

        auto addSpriteDyn = [&](AEGfxTexture* tex, float uvW, float uvH,
            float x, float y, float w, float h,
            float uvOffX, float uvOffY,
            float opacity = 1.0f, float rotation = 0.0f) {
                m_spriteBatch.push_back({ tex, uvW, uvH, x, y, w, h, uvOffX, uvOffY, opacity, rotation });
            };

        const float capWidthLocal = 32.0f;

        // Hover animations - iterate platforms and add hover sprites per-frame
        if (m_hoverAnim) {
            auto addHoverForLevel = [&](const std::vector<Platform>& platforms) {
                for (const auto& p : platforms) {
                    if (!p.active) continue;
                    if (!inView(p.y, p.h * 0.5f)) continue;

                    float leftX = p.x - p.w * 0.5f + capWidthLocal * 0.5f;
                    float rightX = p.x + p.w * 0.5f - capWidthLocal * 0.5f;
                    float midStartX = leftX + capWidthLocal * 0.5f;
                    float midEndX = rightX - capWidthLocal * 0.5f;
                    float midWidth = midEndX - midStartX;
                    if (midWidth > 0.0f) {
                        float midCenterX = (midStartX + midEndX) * 0.5f;
                        addSpriteDyn(m_hoverAnim->GetTexture(),
                            m_hoverAnim->GetSpriteUVWidth(),
                            m_hoverAnim->GetSpriteUVHeight(),
                            midCenterX, p.y - 40.0f, midWidth, p.h,
                            m_hoverAnim->GetUVOffsetX(),
                            m_hoverAnim->GetUVOffsetY());
                    }
                }
            };

            addHoverForLevel(m_level1Platforms);
            addHoverForLevel(m_level2Platforms);
            addHoverForLevel(m_level3Platforms);
            addHoverForLevel(m_bossPlatforms);
            addHoverForLevel(m_wallPlatforms);
            /*addHoverForLevel(m_level3WallPlatforms);*/
        }

        // Checkpoint animations - always dynamic
        if (m_checkpointAnim) {
            for (const auto& cp : m_checkpoints) {
                if (!inView(cp.y, cp.h * 0.5f)) continue;
                addSpriteDyn(m_checkpointAnim->GetTexture(),
                    m_checkpointAnim->GetSpriteUVWidth(),
                    m_checkpointAnim->GetSpriteUVHeight(),
                    cp.x, cp.y, cp.w, cp.h,
                    m_checkpointAnim->GetUVOffsetX(),
                    m_checkpointAnim->GetUVOffsetY());
            }
        }

        // Spike obstacles (animated) - drawn per-frame with per-obstacle sprites
        // Draw regardless of active flag (inactive spikes show as "off" state)
        auto drawObstacles = [&](const std::vector<PlatformObstacle>& obstacles) {
            for (const auto& o : obstacles) {
                if (!o.isSpike || !o.sprite) continue;
                if (!inView(o.y, o.h * 0.5f)) continue;
                addSpriteDyn(o.sprite->GetTexture(),
                    o.sprite->GetSpriteUVWidth(),
                    o.sprite->GetSpriteUVHeight(),
                    o.x, o.y, o.w, o.h,
                    o.sprite->GetUVOffsetX(),
                    o.sprite->GetUVOffsetY());
            }
        };
        drawObstacles(m_level1Obstacles);
        drawObstacles(m_level2Obstacles);
        drawObstacles(m_level3Obstacles);

        // Flush the dynamic sprite batch (sort & submit)
        if (!m_spriteBatch.empty()) {
            std::sort(m_spriteBatch.begin(), m_spriteBatch.end(),
                [](const QueuedSprite& a, const QueuedSprite& b) {
                    if (a.texture != b.texture) return a.texture < b.texture;
                    if (a.uvW != b.uvW) return a.uvW < b.uvW;
                    return a.uvH < b.uvH;
                });

            size_t idx = 0;
            MeshManager& mmLocal = MeshManager::Get();
            while (idx < m_spriteBatch.size()) {
                const QueuedSprite& first = m_spriteBatch[idx];
                mmLocal.BeginBatch(first.texture, first.uvW, first.uvH);
                do {
                    SpriteBatchItem item;
                    item.x = m_spriteBatch[idx].x;
                    item.y = m_spriteBatch[idx].y;
                    item.width = m_spriteBatch[idx].w;
                    item.height = m_spriteBatch[idx].h;
                    item.uvOffsetX = m_spriteBatch[idx].uvOffX;
                    item.uvOffsetY = m_spriteBatch[idx].uvOffY;
                    item.opacity = m_spriteBatch[idx].opacity;
                    item.rotation = m_spriteBatch[idx].rotation;
                    mmLocal.QueueSprite(item);
                    ++idx;
                } while (idx < m_spriteBatch.size() &&
                    m_spriteBatch[idx].texture == first.texture &&
                    m_spriteBatch[idx].uvW == first.uvW &&
                    m_spriteBatch[idx].uvH == first.uvH);
                mmLocal.EndBatch();
            }
            m_spriteBatch.clear();
        }
    }

    // "Press E to save game" prompt (drawn after batch, requires immediate text rendering)
    for (const auto& cp : m_checkpoints) {
        float playerLeft  = player.pos.x - player.width  * 0.5f;
        float playerRight = player.pos.x + player.width  * 0.5f;
        float playerTop   = player.pos.y + player.height * 0.5f;
        float playerBottom= player.pos.y - player.height * 0.5f;

        float rangeLeft   = cp.x - cp.w * 1.0f;
        float rangeRight  = cp.x + cp.w * 1.0f;
        float rangeTop    = cp.y + cp.h * 1.0f;
        float rangeBottom = cp.y - cp.h * 1.0f;

        bool inRangeX = (playerRight >= rangeLeft)  && (playerLeft  <= rangeRight);
        bool inRangeY = (playerTop   >= rangeBottom) && (playerBottom <= rangeTop);

        if (inRangeX && inRangeY) {
            float windowWidth  = (float)AEGfxGetWindowWidth();
            float windowHeight = (float)AEGfxGetWindowHeight();

            float screenX = (cp.x - camX) / (windowWidth * 0.5f);
            float screenY = (cp.y + cp.h + 20.0f - camY) / (windowHeight * 0.5f);

            if (screenX >  0.5f) screenX =  0.5f;
            if (screenX < -0.9f) screenX = -0.9f;

            AEGfxPrint(g_FontSmall, "Press E to save game", screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            break; // only show for nearest checkpoint
        }
    }

    // --------------------------------------------------------------------
    // 2. Buttons (batched) - update per-button animations, then batch draw
    // --------------------------------------------------------------------
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Reuse class-level sprite batch vector (avoids reallocation each frame)
    m_spriteBatch.clear();

    auto addSprite = [&](AEGfxTexture* tex, float uvW, float uvH,
        float x, float y, float w, float h,
        float uvOffX, float uvOffY,
        float opacity = 1.0f, float rotation = 0.0f) {
            m_spriteBatch.push_back({ tex, uvW, uvH, x, y, w, h, uvOffX, uvOffY, opacity, rotation });
        };

    // Helper to sort and draw a batch
    auto flushBatch = [&]() {
        if (m_spriteBatch.empty()) return;
        std::sort(m_spriteBatch.begin(), m_spriteBatch.end(),
            [](const QueuedSprite& a, const QueuedSprite& b) {
                if (a.texture != b.texture) return a.texture < b.texture;
                if (a.uvW != b.uvW) return a.uvW < b.uvW;
                return a.uvH < b.uvH;
            });

        size_t i = 0;
        while (i < m_spriteBatch.size()) {
            const QueuedSprite& first = m_spriteBatch[i];
            mm.BeginBatch(first.texture, first.uvW, first.uvH);
            do {
                SpriteBatchItem item;
                item.x = m_spriteBatch[i].x;
                item.y = m_spriteBatch[i].y;
                item.width = m_spriteBatch[i].w;
                item.height = m_spriteBatch[i].h;
                item.uvOffsetX = m_spriteBatch[i].uvOffX;
                item.uvOffsetY = m_spriteBatch[i].uvOffY;
                item.opacity = m_spriteBatch[i].opacity;
                item.rotation = m_spriteBatch[i].rotation;
                mm.QueueSprite(item);
                ++i;
            } while (i < m_spriteBatch.size() &&
                m_spriteBatch[i].texture == first.texture &&
                m_spriteBatch[i].uvW == first.uvW &&
                m_spriteBatch[i].uvH == first.uvH);
            mm.EndBatch();
        }
        m_spriteBatch.clear();
        };

    auto collectButtons = [&](std::vector<PlatformButton>& buttons, const std::vector<Platform>& platforms) {
        for (auto& btn : buttons) {
            if (!btn.buttonSprite) continue;

            bool isActive = false;
            if (!btn.platformIndices.empty()) {
                int idx = btn.platformIndices[0];
                if (idx >= 0 && idx < (int)platforms.size())
                    isActive = platforms[idx].active;
            }

            // First-draw initialisation
            if (!btn.spriteInitialized) {
                btn.buttonSprite->Play(isActive ? "on" : "off");
                btn.prevState = isActive;
                btn.spriteInitialized = true;
            }

            // State change
            if (isActive != btn.prevState) {
                if (isActive)
                    btn.buttonSprite->Play("transition");
                else
                    btn.buttonSprite->Play("off");
                btn.prevState = isActive;
            }

            // Advance animation
            btn.buttonSprite->Update(dt);
            if (btn.buttonSprite->GetCurrentClip() == "transition" && !btn.buttonSprite->IsPlaying())
                btn.buttonSprite->Play("on");

            // Add to batch using current UV offsets
            addSprite(btn.buttonSprite->GetTexture(),
                btn.buttonSprite->GetSpriteUVWidth(),
                btn.buttonSprite->GetSpriteUVHeight(),
                btn.x, btn.y, btn.w, btn.h,
                btn.buttonSprite->GetUVOffsetX(),
                btn.buttonSprite->GetUVOffsetY());
        }
    };

    collectButtons(m_level1Buttons, m_level1Platforms);
    collectButtons(m_level2Buttons, m_level2Platforms);
    collectButtons(m_level3Buttons, m_level3Platforms);

    // --------------------------------------------------------------------
    // 3. Computers
    // --------------------------------------------------------------------
    auto collectComputers = [&](std::vector<PlatformComputer>& computers, const std::vector<PlatformLaser>& lasers)
    {
        for (auto& comp : computers)
        {
            if (!comp.computerSprite) continue;

            bool isActive = comp.beamActive;            

            // initialize first draw
            if (!comp.spriteInitialized)
            {
                comp.computerSprite->Play(isActive ? "on" : "off");
                comp.prevState = isActive;
                comp.spriteInitialized = true;
            }

            // state change
            if (isActive != comp.prevState)
            {
                if (isActive)
                {
                    comp.computerSprite->Play("transition");
                }
                else
                {
                    comp.computerSprite->Play("off");
                }
                comp.prevState = isActive;
            }

            // advance animation
            comp.computerSprite->Update(dt);

            if (comp.computerSprite->GetCurrentClip() == "transition" && !comp.computerSprite->IsPlaying())
            {
                comp.computerSprite->Play("on");

                if (comp.beamActive)
                {
                    comp.pendingCameraPan = true;
                }
            }

            // sync indicator w/ computer
            auto syncIndicator = [&](SpriteSheet* ind)
            {
                    if (!ind) return;

                    std::string currentClip = comp.computerSprite->GetCurrentClip();
                    if (currentClip != ind->GetCurrentClip())
                    {
                        ind->Play(currentClip.c_str());
                    }
                    ind->Update(dt);
            };

            syncIndicator(comp.indicatorLeft.get());
            syncIndicator(comp.indicatorRight.get());

            // draw computer sprite
            addSprite(comp.computerSprite->GetTexture(),
                      comp.computerSprite->GetSpriteUVWidth(),
                      comp.computerSprite->GetSpriteUVHeight(),
                      comp.x, comp.y, comp.w, comp.h,
                      comp.computerSprite->GetUVOffsetX(),
                      comp.computerSprite->GetUVOffsetY());

            // draw indicators when visible
            if (comp.indicatorLeft && comp.indicatorsVisible)
            {
                addSprite(comp.indicatorLeft->GetTexture(),
                    comp.indicatorLeft->GetSpriteUVWidth(),
                    comp.indicatorLeft->GetSpriteUVHeight(),
                    comp.beamX1, comp.beamY1, comp.indW, comp.indH,
                    comp.indicatorLeft->GetUVOffsetX(),
                    comp.indicatorLeft->GetUVOffsetY());
            }

            if (comp.indicatorRight && comp.indicatorsVisible)
            {
                addSprite(comp.indicatorRight->GetTexture(),
                    comp.indicatorRight->GetSpriteUVWidth(),
                    comp.indicatorRight->GetSpriteUVHeight(),
                    comp.beamX2, comp.beamY2, comp.indW, comp.indH,
                    comp.indicatorRight->GetUVOffsetX(),
                    comp.indicatorRight->GetUVOffsetY());
            }
        }
    };

    collectComputers(m_level3Computers, m_level3Lasers);

    // Draw batched buttons
    flushBatch();

    // Draw immediate "Press E" prompts for buttons when player overlaps
    auto drawButtonPrompts = [&](const std::vector<PlatformButton>& buttons) {
        for (const auto& btn : buttons) {
            float btnLeft = btn.x - btn.w * 0.5f;
            float btnRight = btn.x + btn.w * 0.5f;
            float btnTop = btn.y + btn.h * 0.5f;
            float btnBottom = btn.y - btn.h * 0.5f;
            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerBottom = player.pos.y - player.height * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;

            bool overlapX = (playerRight >= btnLeft) && (playerLeft <= btnRight);
            bool overlapY = (playerTop >= btnBottom) && (playerBottom <= btnTop);
            if (overlapX && overlapY) {
                float windowWidth = (float)AEGfxGetWindowWidth();
                float windowHeight = (float)AEGfxGetWindowHeight();
                float screenX = (btn.x - camX) / (windowWidth * 0.5f);
                float screenY = (btn.y + btn.h + 20.0f - camY) / (windowHeight * 0.5f);
                if (screenX > 0.5f)  screenX = 0.5f;
                if (screenX < -0.9f) screenX = -0.9f;
                AEGfxPrint(g_FontSmall, btn.btnPrompt.c_str(), screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    };

    drawButtonPrompts(m_level1Buttons);
    drawButtonPrompts(m_level2Buttons);
    drawButtonPrompts(m_level3Buttons);

    // Draw immediate "Press E" prompts for computers when player overlaps
    auto drawComputerPrompts = [&](const std::vector<PlatformComputer>& computers) {
        for (const auto& comp : computers) {

            float compLeft = comp.x - comp.w * 0.5f;
            float compRight = comp.x + comp.w * 0.5f;
            float compTop = comp.y + comp.h * 0.5f;
            float compBot = comp.y - comp.h * 0.5f;

            float playerLeft = player.pos.x - player.width * 0.5f;
            float playerRight = player.pos.x + player.width * 0.5f;
            float playerTop = player.pos.y + player.height * 0.5f;
            float playerBot = player.pos.y - player.height * 0.5f;

            bool overlapX = (playerRight >= compLeft) && (playerLeft <= compRight);
            bool overlapY = (playerTop >= compBot) && (playerBot <= compTop);

            if (overlapX && overlapY) {
                float windowWidth = (float)AEGfxGetWindowWidth();
                float windowHeight = (float)AEGfxGetWindowHeight();
                float screenX = (comp.x - camX) / (windowWidth * 0.5f);
                float screenY = (comp.y + comp.h + 20.0f - camY) / (windowHeight * 0.5f);
                if (screenX > 0.5f)  screenX = 0.5f;
                if (screenX < -0.9f) screenX = -0.9f;
                AEGfxPrint(g_FontSmall, "Press E to toggle lasers", screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    };

    drawComputerPrompts(m_level3Computers);

    // --------------------------------------------------------------------
    // 4. Lasers
    // --------------------------------------------------------------------
    auto collectLasers = [&](const std::vector<PlatformLaser>& lasers) {
        for (const auto& ls : lasers) {
            if (!ls.laserActive) continue;
            float laserHeight = ls.y1 - ls.y2;
            float laserCenter = (ls.y1 + ls.y2) * 0.5f;
            if (!inView(laserCenter, laserHeight * 0.5f)) continue;
            mm.DrawTexturedLine(m_laserTex,
                ls.x1, ls.y1,
                ls.x2, ls.y2,
                ls.w, 64.0f);
        }
    };

    collectLasers(m_level2Lasers);
    collectLasers(m_level3Lasers);

    auto drawBeams = [&](const std::vector<PlatformComputer>& computers) {
        for (const auto& comp : computers) {
            if (!comp.beamVisible) continue;
            float centerY = (comp.beamStartY + comp.beamEndY) * 0.5f;
            float halfHeight = fabs(comp.beamStartY - comp.beamEndY) * 0.5f;
            if (!inView(centerY, halfHeight)) continue;
            mm.DrawTexturedLine(m_laserTex,
                comp.beamStartX, comp.beamStartY,
                comp.beamEndX, comp.beamEndY,
                comp.beamW, 64.0f);
        }
    };

    drawBeams(m_level3Computers);

    // --------------------------------------------------------------------
    // 5. Boss door 
    // --------------------------------------------------------------------
    if (m_bossDoorLoaded)
    {
        if (m_bossDoor.doorTex && inView(m_bossDoor.y, m_bossDoor.h * 0.5f))
            mm.DrawTexturedSquare(m_bossDoor.doorTex,
                m_bossDoor.x, m_bossDoor.y,
                m_bossDoor.w, m_bossDoor.h, 1.0f);

        if (m_bossDoor.liftAnim)
        {
            float drawY = m_bossDoor.liftY;
            if (inView(drawY, m_bossDoor.liftH * 0.5f))
                mm.DrawSpriteSheet(*m_bossDoor.liftAnim,
                    m_bossDoor.liftX, drawY,
                    m_bossDoor.liftW, m_bossDoor.liftH);
        }

        if (m_bossDoor.playerNear && !m_liftSeq.active)
        {
            float windowWidth  = (float)AEGfxGetWindowWidth();
            float windowHeight = (float)AEGfxGetWindowHeight();
            float screenX = (m_bossDoor.x - camX) / (windowWidth  * 0.5f);
            float screenY = (m_bossDoor.y + m_bossDoor.h * 0.5f + 20.0f - camY) / (windowHeight * 0.5f);
            if (screenX >  0.5f) screenX =  0.5f;
            if (screenX < -0.9f) screenX = -0.9f;
            AEGfxPrint(g_FontSmall, m_bossDoor.prompt.c_str(),
                screenX, screenY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // fade overlay
    if (m_liftSeq.fadeAlpha > 0.0f)
    {
        float w = (float)AEGfxGetWindowWidth();
        float h = (float)AEGfxGetWindowHeight();
        mm.DrawSquare(camX, camY, w, h, 0, 0, 0, m_liftSeq.fadeAlpha);
    }

    // --------------------------------------------------------------------
    // 6. Ground (single colored square)
    // --------------------------------------------------------------------
    mm.DrawSquare(0.0f, -350.0f, 1600.0f, 50.0f, 0, 0, 0);
}

// ------------------------------------------------------------------------
// HUD Drawing
// ------------------------------------------------------------------------
void EnvironmentManager::DrawHUD(float camX, float camY, PlayerWeapon weapon)
{
    // Update press timers during draw phase so animations play even during pause
    float dt = (float)AEFrameRateControllerGetFrameTime();
    m_HUD.UpdatePressTimers(dt);
    
    m_HUD.Draw(MeshManager::Get(), camX, camY, weapon);
    LevelIndicator_Draw();
}

// ------------------------------------------------------------------------
// Private background helpers
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateBackground(float cameraY)
{
    int index = GetSectionFromY(cameraY);
    m_currentSection = index;

    float lower = (index == 0) ? 0.0f : m_sectionHeights[index - 1];
    float upper = m_sectionHeights[index];
    float blend = (cameraY - lower) / (upper - lower);

    m_currentColour = (index < BACKGROUND_SECTIONS - 1)
        ? BlendColours(m_backgroundColours[index], m_backgroundColours[index + 1], blend)
        : m_backgroundColours[index];
}

void EnvironmentManager::DrawBackground() const
{
    // Set background color
    AEGfxSetBackgroundColor(m_currentColour.r, m_currentColour.g, m_currentColour.b);
    
    // Draw each parallax layer (layer 0 stretched, layers 1-2 tiled)
    MeshManager& mm = MeshManager::Get();
    const float maxWorldHeight = 10600.0f;  // highest section height
    const float screenWidth = 1600.0f;
    const float screenHeight = 900.0f;
    const float groundY = -350.0f;  // Ground position
    
    for (int i = 0; i < 3; i++) {
        if (m_parallaxLayers[i].texture) {
            // Calculate parallax offset (negative for proper scrolling direction)
            // Divide by 5 to reduce parallax strength
            float offsetY = -m_parallaxY * m_parallaxLayers[i].speed / 10.0f;
            
            if (i == 0) {
                // First layer (back) - stretch from ground to max world height
                float layerHeight = maxWorldHeight - groundY;
                float layerCenterY = groundY + layerHeight * 0.5f + offsetY;
                mm.DrawTexturedSquare(
                    m_parallaxLayers[i].texture,
                    0.0f, layerCenterY,
                    screenWidth, layerHeight
                );
            } else {
                // Layers 1-2 (middle and front) - tile by repeating texture as we scroll
                float tileHeight = screenHeight;  // Tile size
                float startY = offsetY;
                
                // Calculate how many tiles we need based on world movement
                int numTiles = static_cast<int>(maxWorldHeight / tileHeight) + 2;
                
                for (int t = 0; t < numTiles; t++) {
                    float tileY = startY + (t * tileHeight);
                    mm.DrawTexturedSquare(
                        m_parallaxLayers[i].texture,
                        0.0f, tileY,
                        screenWidth, tileHeight
                    );
                }
            }
        }
    }
}

// Draw dark overlay between parallax background and game objects
void EnvironmentManager::DrawBackgroundOverlay(float camX, float camY) const
{
    MeshManager& mm = MeshManager::Get();
    const float screenWidth = 1600.0f;
    const float screenHeight = 900.0f;
    
    // Draw a dark semi-transparent rectangle covering the entire screen
    // Dark gray with 40% opacity (60 out of 255)
    mm.DrawSquare(camX, camY, screenWidth, screenHeight, 30, 30, 30, 0.3f);
}

// ------------------------------------------------------------------------
bool EnvironmentManager::HandleCheckpoint(bool checkpointHit)
{
    bool shouldSave = false;

    if (checkpointHit || m_saveRequested)
    {
        if (!m_checkpointSaved || m_saveRequested)
        {
            m_checkpointSaved = true;
            shouldSave = true;
            m_saveRequested = false;
        }
    }
    else
    {
        m_checkpointSaved = false;
    }

    return shouldSave;
}

void EnvironmentManager::RequestSave()
{
    m_saveRequested = true;
}

bool EnvironmentManager::isSaveRequested()
{
    bool shouldSave = false;
    if (m_saveRequested) {
        shouldSave = true;
        m_saveRequested = false; // reset after checking
    }
    return shouldSave;
}

void EnvironmentManager::SetCheckpointInRange(bool inRange)
{
    m_checkpointInRange = inRange;
}

bool EnvironmentManager::GetCheckpointInRange() const
{
    return m_checkpointInRange;
}

// ------------------------------------------------------------------------
int EnvironmentManager::GetSectionFromY(float y) const
{
    for (int i = 0; i < BACKGROUND_SECTIONS; ++i) {
        if (y <= m_sectionHeights[i])
            return i;
    }
    return BACKGROUND_SECTIONS - 1;
}

const std::vector<PlatformObstacle>& EnvironmentManager::GetCurrentObstacles() const
{
    int currentLevel = m_currentSection + 1;

    switch (currentLevel) {
    case 1:  return m_level1Obstacles;
    case 2:  return m_level2Obstacles;
    case 3:  return m_level3Obstacles;
    default: return m_level1Obstacles;
    }
}

EnvironmentManager::Colour EnvironmentManager::BlendColours(const Colour& a, const Colour& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// ------------------------------------------------------------------------
// Level indicator wrappers
// ------------------------------------------------------------------------
void EnvironmentManager::UpdateLevelIndicator(float dt) { LevelIndicator_Update(dt); }
void EnvironmentManager::DrawLevelIndicator()  const { LevelIndicator_Draw(); }

// ------------------------------------------------------------------------
void EnvironmentManager::Clear()
{
    m_level1Platforms.clear();
    m_level2Platforms.clear();
    m_level3Platforms.clear();
    m_bossPlatforms.clear();

    m_wallPlatforms.clear();
    m_level3WallPlatforms.clear();
    m_level3ToggleWalls.clear();

    m_level1Obstacles.clear();
    m_level2Obstacles.clear();
    m_level3Obstacles.clear();

    m_checkpoints.clear();

    m_level1Buttons.clear();
    m_level2Buttons.clear();
    m_level3Buttons.clear();

    m_level2Lasers.clear();
    m_level3Lasers.clear();

    m_level3Computers.clear();

    m_bossDoor        = BossDoor{};
    m_liftSeq         = BossLiftSequence{};
    m_bossDoorLoaded  = false;
    m_bossRoomMode = false;
    
    // Also clear static cache and mark dirty to avoid stale geometry after a restart/load
    m_staticCache.clear();
    m_staticBatchDirty = true;

}

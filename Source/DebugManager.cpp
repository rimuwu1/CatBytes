/* Start Header ************************************************************************/
/*!
\file DebugManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 05/03/2026
\brief Implements the debug overlay and in-game developer console.

  +-------------------------------------------------------------+
  |  KEY BINDINGS                                               |
  |  F1          Toggle stats overlay  (always)                 |
  |  F2          Open/close console    (always)                 |
  |  F6          Toggle hitbox overlays (always)                |
  |  platformedit [on|off]       Toggle platform index labels    |
  |  selectplatform <L> <N>      Select platform at level L #N   |
  |  setpos <x> <y>              Move selected platform           |
  |  printsel                    Print selected platform info     |
  |  setlength <value>           Set selected platform width      |
  |               - includes lasers & obstacles                 |
  |  0           Toggle debug camera  (always)                 |
  |  L           Manual save          (always)                 |
  |  M           Reset save           (always)                 |
  |                                                             |
  |  While overlay is ON and console is closed:                 |
  |  F3          Kill all alive enemies                         |
  |  F4          Force Win screen                               |
  |  F5          Force Lose screen                              |
  |                                                             |
  |  While console is OPEN:                                     |
  |  Enter       Execute command                                |
  |  Tab         Auto-complete command name                     |
  |  Up / Dn     Browse command history                         |
  |  Backspace   Delete last character                          |
  |  Esc         Close console                                  |
  +-------------------------------------------------------------+

  +-------------------------------------------------------------+
  |  BUILT-IN COMMANDS                                          |
  |  help                  List all commands                    |
  |  clear                 Clear console log                    |
  |  godmode [on|off]     Toggle god mode                       |
  |  noclip [on|off]      Toggle noclip/fly mode                |
  |  kill                  Kill all alive enemies               |
  |  win                   Force Win screen                     |
  |  lose                  Force Lose screen                    |
  |  camera [on|off]      Toggle debug camera mode             |
  |  save                  Trigger manual game save             |
  |  reset                 Reset save file                      |
  |  tp <1|2|3|4|boss>    Teleport to section start            |
  |  tpxy <x> <y>         Teleport to exact world coords       |
  |  hp <value>           Set player HP (float)                 |
  |  section              Print current section/level           |
  |  pos                  Print player position                 |
  |  enemies              List alive enemy count by type        |
  |  hitbox [on|off]      Toggle hitbox overlays                |
  |  physics              Print all current physics values      |
  |  speed   <value|reset> Set PhysicsManager move speed       |
  |  gravity <value|reset> Set PhysicsManager gravity          |
  |  jump    <value|reset> Set PhysicsManager jump force       |
  |  termvel <value|reset> Set PhysicsManager terminal velocity|
  +-------------------------------------------------------------+

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "DebugManager.h"

#ifdef ENABLE_DEBUG_MANAGER

#include "ObjectManager.h"
#include "EnvironmentManager.h"
#include "GameSaveManager.h"
#include "GameStateManager.h"
#include "MeshManager.h"
#include "PhysicsManager.h"
#include "Camera.h"
#include "WinLose.h"
#include "Fonts.h"
#include "Player.h"
#include "PlayerBullet.h"
#include "enemy.h"
#include <sstream>
#include <algorithm>
#include <cctype>

// ============================================================================
// Layout constants
// ============================================================================

// ---- Overlay (top-left, screen-anchored) -----------------------------------
static constexpr float OVL_TEXT_X = -0.90f;
static constexpr float OVL_START_Y = 0.88f;
static constexpr float OVL_LINE_H = 0.09f;
static constexpr float OVL_SCALE = 0.60f;
static constexpr float OVL_PANEL_W = 520.f;
static constexpr float OVL_PANEL_H = 580.f;

// ---- Console (centred, bottom third of screen) -----------------------------
static constexpr float CON_PANEL_W = 900.f;
static constexpr float CON_PANEL_H = 300.f;
static constexpr float CON_TEXT_X = -0.95f;
static constexpr float CON_LOG_TOP_Y = -0.43f;
static constexpr float CON_LINE_H = 0.10f;
static constexpr float CON_INPUT_Y = -0.90f;
static constexpr float CON_HINT_ALPHA = 0.45f;
static constexpr float CON_SCALE = 0.60f;
static constexpr int   CON_MAX_VISIBLE = 4;

// Teleport destinations
static constexpr struct { const char* key; float x; float y; } k_TpSpots[] =
{
    { "1",    0.f,   -120.f },  // first platform of level 1 (center)
    { "2",   0.f, 1810.f },  // first platform of level 2
    { "3",    0.f, 4600.f },  // first platform of level 3
    { "4",      520.f, 9660.f },  // start of level 4 (boss room i think)
    { "boss",   0.f, 7500.f },  // boss arena (uhh i dont think this platform is supposed to be here but)
};
static constexpr int k_TpCount = (int)(sizeof(k_TpSpots) / sizeof(k_TpSpots[0]));

// Default physics values -- used to detect tweaks and highlight them in amber
static constexpr float DEF_SPEED = 400.f;
static constexpr float DEF_GRAVITY = -1300.f;
static constexpr float DEF_JUMP = 650.f;
static constexpr float DEF_TERM = 2000.f;

// ============================================================================
// File-local helpers
// ============================================================================

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool Tweaked(float cur, float def)
{
    return (cur < def - 0.5f || cur > def + 0.5f);
}

// ============================================================================
// Lifecycle
// ============================================================================

void DebugManager::Initialize()
{
    m_GodMode = false;
    m_OverlayOn = false;
    m_HitboxOn = false;   // hitboxes off by default; toggle with F6 or 'hitbox' command
    m_GridOn = false;     // grid off by default; toggle with 'showgrid' command
    m_Noclip = false;
    m_ConsoleOpen = false;
    m_InputBuffer.clear();
    m_AutoHint.clear();
    m_Log.clear();
    m_History.clear();
    m_HistoryIdx = -1;
    m_FrameCount = 0;
    m_FpsTimer = 0.f;
    m_FpsCached = 0.f;
    m_BlinkTimer = 0.f;
    m_CursorBlink = true;
    m_Commands.clear();

    RegisterBuiltinCommands();
    m_Initialized = true;
    Log("Debug console ready. Type 'help' for commands.");
}

void DebugManager::Reset()
{
    m_GodMode = false;
    m_DebugCamera = false;
    m_Noclip = false;
    m_ConsoleOpen = false;
    m_InputBuffer.clear();
    m_AutoHint.clear();
    m_Log.clear();
    m_History.clear();
    m_HistoryIdx = -1;
    Log("Console reset.");
}

// ============================================================================
// Update
// ============================================================================

bool DebugManager::Update(float dt)
{
    if (!m_Initialized) Initialize();

    // FPS counter
    ++m_FrameCount;
    m_FpsTimer += dt;
    if (m_FpsTimer >= 1.f)
    {
        m_FpsCached = (float)m_FrameCount / m_FpsTimer;
        m_FrameCount = 0;
        m_FpsTimer = 0.f;
    }

    // Cursor blink
    m_BlinkTimer += dt;
    if (m_BlinkTimer >= BLINK_RATE)
    {
        m_CursorBlink = !m_CursorBlink;
        m_BlinkTimer = 0.f;
    }

    // F1 -- always toggles stats overlay
    if (AEInputCheckTriggered(AEVK_F1))
        m_OverlayOn = !m_OverlayOn;

    // F2 -- always toggles console
    if (AEInputCheckTriggered(AEVK_F2))
    {
        m_ConsoleOpen = !m_ConsoleOpen;
        if (m_ConsoleOpen)
        {
            m_InputBuffer.clear();
            m_AutoHint.clear();
            m_HistoryIdx = -1;
        }
    }

    // F6 -- always toggles hitbox overlays, independent of the stats overlay
    if (AEInputCheckTriggered(AEVK_F6))
        m_HitboxOn = !m_HitboxOn;

    // 0 - toggle debug camera (always active)
    if (!m_ConsoleOpen && AEInputCheckTriggered(AEVK_0))
    {
        m_DebugCamera = !m_DebugCamera;
        globalCam.debugCam = m_DebugCamera;
        if (!m_DebugCamera)
        {
            globalCam.x = 0.0f;
            globalCam.y = ObjectManager::Get().GetPlayer().pos.y;
        }
    }

    if (!m_ConsoleOpen)
    {
        // L - manual save (only when console is closed)
        if (AEInputCheckTriggered('L'))
        {
            EnvironmentManager::Get().RequestSave();
        }

        // M - reset save (only when console is closed)
        if (AEInputCheckTriggered('M'))
        {
            GameSaveManager::ResetSave();
            GameSaveManager::Notify_Show(GameSaveManager::NotifyType::RESET);
        }
    }

    // Console is modal -- consume all game input while open
    if (m_ConsoleOpen)
    {
        UpdateConsoleInput();
        return true;
    }

    // Hotkeys only when overlay is visible and console is closed
    if (m_OverlayOn)
        HandleHotkeys();

    return false;
}

// ============================================================================
// Draw
// ============================================================================

void DebugManager::Draw(float camX, float camY)
{
    if (m_OverlayOn)   DrawOverlay(camX, camY);
    if (m_ConsoleOpen) DrawConsole(camX, camY);
}

// ============================================================================
// Log
// ============================================================================

void DebugManager::Log(const std::string& msg)
{
    m_Log.push_back(msg);
    while ((int)m_Log.size() > LOG_CAPACITY)
        m_Log.pop_front();
}

// ============================================================================
// Command registration
// ============================================================================

void DebugManager::RegisterCommand(const std::string& name,
    const std::string& usage,
    const std::string& description,
    std::function<void(const std::vector<std::string>&)> handler)
{
    m_Commands[ToLower(name)] = { ToLower(name), usage, description, handler };
}

// ============================================================================
// Built-in commands
// ============================================================================

void DebugManager::RegisterBuiltinCommands()
{
    // ---- help ---------------------------------------------------------------
    RegisterCommand("help", "help", "List all available commands.",
        [this](const std::vector<std::string>&)
        {
            Log("Commands:");
            for (auto it = m_Commands.begin(); it != m_Commands.end(); ++it)
                Log("  " + it->second.usage + " -- " + it->second.description);
        });

    // ---- clear --------------------------------------------------------------
    RegisterCommand("clear", "clear", "Clear the console log.",
        [this](const std::vector<std::string>&) { m_Log.clear(); });

    // ---- godmode ------------------------------------------------------------
    RegisterCommand("godmode", "godmode [on|off]", "Toggle god mode (callers check IsGodModeActive).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_GodMode = (ToLower(args[1]) == "on");
            else
                m_GodMode = !m_GodMode;
            Log(std::string("God mode: ") + (m_GodMode ? "ON" : "OFF"));
        });

    // ---- kill ---------------------------------------------------------------
    RegisterCommand("kill", "kill", "Set isAlive=0 on all enemies.",
        [this](const std::vector<std::string>&)
        {
            auto& enemies = ObjectManager::Get().GetAllEnemies();
            int n = 0;
            for (auto& e : enemies)
                if (e.isAlive) { e.isAlive = 0; ++n; }
            Log("Killed " + std::to_string(n) + " enemies.");
        });

    // ---- win ----------------------------------------------------------------
    RegisterCommand("win", "win", "Force the Win screen.",
        [this](const std::vector<std::string>&)
        {
            textScreenMessage = "You Win";
            GameStateManager::Get().next = GS_WINLOSE;
            Log("Forcing WIN...");
        });

    // ---- lose ---------------------------------------------------------------
    RegisterCommand("lose", "lose", "Force the Lose screen.",
        [this](const std::vector<std::string>&)
        {
            textScreenMessage = "You Lose";
            GameStateManager::Get().next = GS_WINLOSE;
            Log("Forcing LOSE...");
        });

    // ---- camera --------------------------------------------------------------
    RegisterCommand("camera", "camera [on|off]", "Toggle debug camera mode.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_DebugCamera = (ToLower(args[1]) == "on");
            else
                m_DebugCamera = !m_DebugCamera;
            globalCam.debugCam = m_DebugCamera;
            if (!m_DebugCamera)
            {
                globalCam.x = 0.0f;
                globalCam.y = ObjectManager::Get().GetPlayer().pos.y;
            }
            Log(std::string("Debug camera: ") + (m_DebugCamera ? "ON" : "OFF"));
        });

    // ---- save ----------------------------------------------------------------
    RegisterCommand("save", "save", "Trigger manual game save.",
        [this](const std::vector<std::string>&)
        {
            EnvironmentManager::Get().RequestSave();
            Log("Manual save triggered.");
        });

    // ---- setlength ------------------------------------------------------
    RegisterCommand("setlength", "setlength <value>",
        "Set selected platform width (length). Center position stays fixed.",
        [this](const std::vector<std::string>& args)
        {
            if (!m_SelectedPlatform) {
                Log("No platform selected. Use selectplatform <level> <index>.");
                return;
            }
            if (args.size() < 2) { Log("Usage: setlength <value>"); return; }
            try {
                float len = std::stof(args[1]);
                if (len <= 0.f) { Log("Length must be greater than 0."); return; }
                m_SelectedPlatform->w = len;
                EnvironmentManager::Get().MarkStaticDirty();
                char buf[64];
                snprintf(buf, sizeof(buf),
                    "Platform L%d #%d width set to %.1f",
                    m_SelectedPlatformLevel, m_SelectedPlatformIdx, (double)len);
                Log(buf);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- reset ---------------------------------------------------------------
    RegisterCommand("reset", "reset", "Reset save file.",
        [this](const std::vector<std::string>&)
        {
            GameSaveManager::ResetSave();
            GameSaveManager::Notify_Show(GameSaveManager::NotifyType::RESET);
            Log("Save file reset.");
        });

    // ---- tp -----------------------------------------------------------------
    RegisterCommand("tp", "tp <1|2|3|4|boss>", "Teleport to section start. Zeroes velocity.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: tp <1|2|3|4|boss>"); return; }
            Player& p = ObjectManager::Get().GetPlayer();
            const std::string& dest = args[1];
            for (int i = 0; i < k_TpCount; ++i)
            {
                if (dest == k_TpSpots[i].key)
                {
                    p.pos.x = k_TpSpots[i].x;
                    p.pos.y = k_TpSpots[i].y;
                    p.vel.x = 0.f;
                    p.vel.y = 0.f;
                    p.grounded = 0;
                    Log(std::string("Teleported to section ") + dest
                        + " (" + std::to_string((int)k_TpSpots[i].x)
                        + ", " + std::to_string((int)k_TpSpots[i].y) + ")");
                    return;
                }
            }
            Log("Unknown destination '" + dest + "'. Options: 1 2 3 4 boss");
        });

    // ---- tpxy ---------------------------------------------------------------
    RegisterCommand("tpxy", "tpxy <x> <y>", "Teleport player to exact world coordinates.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 3) { Log("Usage: tpxy <x> <y>"); return; }
            try {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                Player& p = ObjectManager::Get().GetPlayer();
                p.pos.x = x;
                p.pos.y = y;
                p.vel.x = 0.f;
                p.vel.y = 0.f;
                p.grounded = 0;
                char buf[64];
                snprintf(buf, sizeof(buf), "Teleported to (%.1f, %.1f)", (double)x, (double)y);
                Log(buf);
            }
            catch (...) { Log("Invalid coordinates."); }
        });

    // ---- noclip -------------------------------------------------------------
    RegisterCommand("noclip", "noclip [on|off]", "Toggle noclip/fly mode (disables gravity, free movement).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_Noclip = (ToLower(args[1]) == "on");
            else
                m_Noclip = !m_Noclip;
            Log(std::string("Noclip: ") + (m_Noclip ? "ON" : "OFF"));
        });


    // ---- hp -----------------------------------------------------------------
    RegisterCommand("hp", "hp <value>", "Set player HP (float).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: hp <value>"); return; }
            try
            {
                ObjectManager::Get().GetPlayer().hp = std::stof(args[1]);
                Log("Player HP = " + args[1]);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- section ------------------------------------------------------------
    RegisterCommand("section", "section", "Print current section and level.",
        [this](const std::vector<std::string>&)
        {
            int s = EnvironmentManager::Get().GetCurrentSection();
            Log("Section " + std::to_string(s) + "  ->  Level " + std::to_string(s + 1));
        });

    // ---- pos ----------------------------------------------------------------
    RegisterCommand("pos", "pos", "Print player world position.",
        [this](const std::vector<std::string>&)
        {
            const Player& p = ObjectManager::Get().GetPlayer();
            char buf[64];
            snprintf(buf, sizeof(buf), "Player pos (%.1f, %.1f)", (double)p.pos.x, (double)p.pos.y);
            Log(buf);
        });

    // ---- enemies ------------------------------------------------------------
    RegisterCommand("enemies", "enemies", "Print alive enemy counts by type.",
        [this](const std::vector<std::string>&)
        {
            const auto& enemies = ObjectManager::Get().GetAllEnemies();
            int easy = 0, hard = 0, boss = 0, dead = 0;
            for (const auto& e : enemies)
            {
                if (!e.isAlive) { ++dead; continue; }
                switch (e.type)
                {
                case EnemyType::Easy: ++easy; break;
                case EnemyType::Hard: ++hard; break;
                case EnemyType::Boss: ++boss; break;
                }
            }
            Log("Alive -- Easy:" + std::to_string(easy)
                + " Hard:" + std::to_string(hard)
                + " Boss:" + std::to_string(boss)
                + "  |  Dead:" + std::to_string(dead));
        });

    // ---- hitbox -------------------------------------------------------------
    RegisterCommand("hitbox", "hitbox [on|off]",
        "Toggle world-space hitbox/arrow overlays (also F6).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_HitboxOn = (ToLower(args[1]) == "on");
            else
                m_HitboxOn = !m_HitboxOn;
            Log(std::string("Hitbox overlays: ") + (m_HitboxOn ? "ON" : "OFF"));
        });

    // ---- platformedit ---------------------------------------------------
    RegisterCommand("platformedit", "platformedit [on|off]",
        "Toggle level/index labels over all platforms.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_PlatformEditMode = (ToLower(args[1]) == "on");
            else
                m_PlatformEditMode = !m_PlatformEditMode;
            m_SelectedPlatform      = nullptr;
            m_SelectedPlatformLevel = -1;
            m_SelectedPlatformIdx   = -1;
            Log(std::string("Platform edit mode: ") +
                (m_PlatformEditMode ? "ON — labels visible, use selectplatform <L> <N>" : "OFF"));
        });

    // ---- selectplatform -------------------------------------------------
    RegisterCommand("selectplatform", "selectplatform <level> <index>",
        "Select a platform by level (1-4) and index for editing.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 3) { Log("Usage: selectplatform <level> <index>"); return; }
            try {
                int lvl = std::stoi(args[1]);
                int idx = std::stoi(args[2]);

                auto& env = EnvironmentManager::Get();
                std::vector<Platform>* levelPtrs[] = {
                    const_cast<std::vector<Platform>*>(&env.GetLevel1Platforms()),
                    const_cast<std::vector<Platform>*>(&env.GetLevel2Platforms()),
                    const_cast<std::vector<Platform>*>(&env.GetLevel3Platforms()),
                    const_cast<std::vector<Platform>*>(&env.GetBossPlatforms())
                };

                if (lvl < 1 || lvl > 4) { Log("Level must be 1-4."); return; }
                auto& plats = *levelPtrs[lvl - 1];
                if (idx < 0 || idx >= (int)plats.size()) {
                    Log("Index out of range. Max: " + std::to_string((int)plats.size() - 1));
                    return;
                }

                m_SelectedPlatform      = &plats[idx];
                m_SelectedPlatformLevel = lvl;
                m_SelectedPlatformIdx   = idx;
                char buf[96];
                snprintf(buf, sizeof(buf),
                    "Selected L%d #%d  pos(%.1f, %.1f)  size(%.0f x %.0f)  active=%d",
                    lvl, idx,
                    (double)plats[idx].x, (double)plats[idx].y,
                    (double)plats[idx].w, (double)plats[idx].h,
                    (int)plats[idx].active);
                Log(buf);
            }
            catch (...) { Log("Invalid arguments."); }
        });

    // ---- setpos ---------------------------------------------------------
    RegisterCommand("setpos", "setpos <x> <y>",
        "Move selected platform to new world position.",
        [this](const std::vector<std::string>& args)
        {
            if (!m_SelectedPlatform) {
                Log("No platform selected. Use selectplatform <level> <index>.");
                return;
            }
            if (args.size() < 3) { Log("Usage: setpos <x> <y>"); return; }
            try {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                m_SelectedPlatform->x = x;
                m_SelectedPlatform->y = y;
                EnvironmentManager::Get().MarkStaticDirty();
                char buf[64];
                snprintf(buf, sizeof(buf),
                    "Platform L%d #%d moved to (%.1f, %.1f)",
                    m_SelectedPlatformLevel, m_SelectedPlatformIdx,
                    (double)x, (double)y);
                Log(buf);
            }
            catch (...) { Log("Invalid coordinates."); }
        });

    // ---- printsel -------------------------------------------------------
    RegisterCommand("printsel", "printsel",
        "Print selected platform info.",
        [this](const std::vector<std::string>&)
        {
            if (!m_SelectedPlatform) { Log("No platform selected."); return; }
            char buf[96];
            snprintf(buf, sizeof(buf),
                "L%d #%d  pos(%.1f, %.1f)  size(%.0f x %.0f)  active=%d",
                m_SelectedPlatformLevel, m_SelectedPlatformIdx,
                (double)m_SelectedPlatform->x, (double)m_SelectedPlatform->y,
                (double)m_SelectedPlatform->w, (double)m_SelectedPlatform->h,
                (int)m_SelectedPlatform->active);
            Log(buf);
        });

    // ---- showgrid -----------------------------------------------------------
    RegisterCommand("showgrid", "showgrid [on|off]",
        "Toggle spatial grid cell visualization.",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() >= 2)
                m_GridOn = (ToLower(args[1]) == "on");
            else
                m_GridOn = !m_GridOn;
            Log(std::string("Grid visualization: ") + (m_GridOn ? "ON" : "OFF"));
        });

    // =========================================================================
    // Physics commands -- all route through PhysicsManager
    // =========================================================================

    // ---- physics (status readout) -------------------------------------------
    RegisterCommand("physics", "physics", "Print all current PhysicsManager values.",
        [this](const std::vector<std::string>&)
        {
            const PhysicsManager& pm = PhysicsManager::Get();
            char buf[96];
            Log("-- PhysicsManager --");
            snprintf(buf, sizeof(buf), "  speed   = %.1f u/s%s",
                (double)pm.GetMoveSpeed(), Tweaked(pm.GetMoveSpeed(), DEF_SPEED) ? "  [TWEAKED]" : ""); Log(buf);
            snprintf(buf, sizeof(buf), "  gravity = %.1f u/s2%s",
                (double)pm.GetGravity(), Tweaked(pm.GetGravity(), DEF_GRAVITY) ? "  [TWEAKED]" : ""); Log(buf);
            snprintf(buf, sizeof(buf), "  jump    = %.1f u/s%s",
                (double)pm.GetJumpForce(), Tweaked(pm.GetJumpForce(), DEF_JUMP) ? "  [TWEAKED]" : ""); Log(buf);
            snprintf(buf, sizeof(buf), "  termvel = %.1f u/s%s",
                (double)pm.GetTerminalVel(), Tweaked(pm.GetTerminalVel(), DEF_TERM) ? "  [TWEAKED]" : ""); Log(buf);
        });

    // ---- speed --------------------------------------------------------------
    RegisterCommand("speed", "speed <value|reset>",
        "Set PhysicsManager move speed (reset = 400 u/s).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: speed <value|reset>"); return; }
            try
            {
                float val = (ToLower(args[1]) == "reset") ? DEF_SPEED : std::stof(args[1]);
                if (val <= 0.f) val = DEF_SPEED;
                PhysicsManager::Get().SetMoveSpeed(val);
                char buf[64];
                snprintf(buf, sizeof(buf), "MoveSpeed = %.1f u/s", (double)val);
                Log(buf);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- gravity ------------------------------------------------------------
    RegisterCommand("gravity", "gravity <value|reset>",
        "Set PhysicsManager gravity (negative = downward, reset = -1300).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: gravity <value|reset>"); return; }
            try
            {
                float val = (ToLower(args[1]) == "reset") ? DEF_GRAVITY : std::stof(args[1]);
                PhysicsManager::Get().SetGravity(val);
                char buf[64];
                snprintf(buf, sizeof(buf), "Gravity = %.1f u/s2", (double)val);
                Log(buf);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- jump ---------------------------------------------------------------
    RegisterCommand("jump", "jump <value|reset>",
        "Set PhysicsManager jump force (positive = upward, reset = 650).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: jump <value|reset>"); return; }
            try
            {
                float val = (ToLower(args[1]) == "reset") ? DEF_JUMP : std::stof(args[1]);
                PhysicsManager::Get().SetJumpForce(val);
                char buf[64];
                snprintf(buf, sizeof(buf), "JumpForce = %.1f u/s", (double)val);
                Log(buf);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- termvel ------------------------------------------------------------
    RegisterCommand("termvel", "termvel <value|reset>",
        "Set PhysicsManager terminal velocity magnitude (reset = 2000).",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2) { Log("Usage: termvel <value|reset>"); return; }
            try
            {
                float val = (ToLower(args[1]) == "reset") ? DEF_TERM : std::stof(args[1]);
                if (val < 0.f) val = -val;
                PhysicsManager::Get().SetTerminalVel(val);
                char buf[64];
                snprintf(buf, sizeof(buf), "TerminalVel = %.1f u/s", (double)val);
                Log(buf);
            }
            catch (...) { Log("Invalid value."); }
        });

    // ---- grid ----------------------------------------------------------------
    RegisterCommand("grid", "grid",
        "Show spatial grid info (bounds, cell size, cell count).",
        [this](const std::vector<std::string>&)
        {
            const SpatialGrid& grid = EnvironmentManager::Get().GetSpatialGrid();
            char buf[128];
            Log("-- SpatialGrid --");
            snprintf(buf, sizeof(buf), "  cells: %d", grid.GetCellCount());
            Log(buf);
            snprintf(buf, sizeof(buf), "  cellHeight: %.1f", (double)grid.GetCellHeight());
            Log(buf);
            snprintf(buf, sizeof(buf), "  bounds: Y [%.1f, %.1f]", (double)grid.GetMinY(), (double)grid.GetMaxY());
            Log(buf);
        });

    // ---- gridcells -----------------------------------------------------------
    RegisterCommand("gridcells", "gridcells",
        "Print object counts in each spatial grid cell (may be verbose).",
        [this](const std::vector<std::string>&)
        {
            const SpatialGrid& grid = EnvironmentManager::Get().GetSpatialGrid();
            grid.DebugPrintCellCounts(*this);
        });

    // ---- gridnear ------------------------------------------------------------
    RegisterCommand("gridnear", "gridnear [y]",
        "Show nearby objects for Y position (uses player Y if omitted).",
        [this](const std::vector<std::string>& args)
        {
            const Player& p = ObjectManager::Get().GetPlayer();
            const SpatialGrid& grid = EnvironmentManager::Get().GetSpatialGrid();
            float y = p.pos.y;
            float h = p.height;
            if (args.size() >= 2)
            {
                try { y = std::stof(args[1]); }
                catch (...) { Log("Invalid Y value."); return; }
            }
            grid.DebugPrintNearby(y, h, *this);
        });
}

// ============================================================================
// Console input
// ============================================================================

void DebugManager::UpdateConsoleInput()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        m_ConsoleOpen = false;
        m_InputBuffer.clear();
        m_AutoHint.clear();
        return;
    }

    if (AEInputCheckTriggered(AEVK_RETURN))
    {
        SubmitCommand();
        return;
    }

    if (AEInputCheckTriggered(AEVK_TAB) && !m_AutoHint.empty())
    {
        m_InputBuffer = m_AutoHint;
        m_AutoHint.clear();
        return;
    }

    if (AEInputCheckTriggered(AEVK_BACK) && !m_InputBuffer.empty())
    {
        m_InputBuffer.pop_back();
        RefreshAutoHint();
        return;
    }

    if (AEInputCheckTriggered(AEVK_UP) && !m_History.empty())
    {
        m_HistoryIdx = std::min((int)m_History.size() - 1, m_HistoryIdx + 1);
        m_InputBuffer = m_History[(int)m_History.size() - 1 - m_HistoryIdx];
        RefreshAutoHint();
        return;
    }

    if (AEInputCheckTriggered(AEVK_DOWN))
    {
        if (m_HistoryIdx > 0)
        {
            --m_HistoryIdx;
            m_InputBuffer = m_History[(int)m_History.size() - 1 - m_HistoryIdx];
        }
        else
        {
            m_HistoryIdx = -1;
            m_InputBuffer.clear();
        }
        RefreshAutoHint();
        return;
    }

    // Printable character input via AEEngine virtual keys.
    // Includes '-' and '.' so negative floats (e.g. gravity -1300) can be typed.
    static const struct { unsigned int vk; char lo; char hi; } s_CharMap[] =
    {
        {'A','a','A'}, {'B','b','B'}, {'C','c','C'}, {'D','d','D'},
        {'E','e','E'}, {'F','f','F'}, {'G','g','G'}, {'H','h','H'},
        {'I','i','I'}, {'J','j','J'}, {'K','k','K'}, {'L','l','L'},
        {'M','m','M'}, {'N','n','N'}, {'O','o','O'}, {'P','p','P'},
        {'Q','q','Q'}, {'R','r','R'}, {'S','s','S'}, {'T','t','T'},
        {'U','u','U'}, {'V','v','V'}, {'W','w','W'}, {'X','x','X'},
        {'Y','y','Y'}, {'Z','z','Z'},
        {'0','0',')'}, {'1','1','!'}, {'2','2','@'}, {'3','3','#'},
        {'4','4','$'}, {'5','5','%'}, {'6','6','^'}, {'7','7','&'},
        {'8','8','*'}, {'9','9','('},
        {AEVK_MINUS,  '-', '_'},
        {AEVK_PERIOD, '.', '>'},
        {AEVK_SPACE,      ' ', ' '},
    };

    bool shift = AEInputCheckCurr(AEVK_LSHIFT);
    for (const auto& m : s_CharMap)
    {
        if (AEInputCheckTriggered(static_cast<u8>(m.vk)))
        {
            if (m_InputBuffer.size() < 80)
            {
                m_InputBuffer += shift ? m.hi : m.lo;
                RefreshAutoHint();
            }
            break;
        }
    }
}

void DebugManager::SubmitCommand()
{
    std::string line = m_InputBuffer;
    m_InputBuffer.clear();
    m_AutoHint.clear();
    m_HistoryIdx = -1;

    if (line.empty()) return;

    Log("> " + line);

    m_History.push_back(line);
    while ((int)m_History.size() > HIST_CAPACITY)
        m_History.pop_front();

    auto tokens = Tokenize(line);
    if (tokens.empty()) return;

    auto it = m_Commands.find(ToLower(tokens[0]));
    if (it != m_Commands.end())
        it->second.handler(tokens);
    else
        Log("Unknown command '" + tokens[0] + "'. Type 'help'.");
}

void DebugManager::RefreshAutoHint()
{
    m_AutoHint.clear();
    if (m_InputBuffer.empty()) return;
    if (m_InputBuffer.find(' ') != std::string::npos) return;

    std::string prefix = ToLower(m_InputBuffer);
    for (auto it = m_Commands.begin(); it != m_Commands.end(); ++it)
    {
        const std::string& k = it->first;
        if (k.size() >= prefix.size() && k.substr(0, prefix.size()) == prefix)
        {
            m_AutoHint = k;
            return;
        }
    }
}

std::vector<std::string> DebugManager::Tokenize(const std::string& s)
{
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// ============================================================================
// Hotkeys (only when overlay visible, console closed)
// ============================================================================

void DebugManager::HandleHotkeys()
{
    if (AEInputCheckTriggered(AEVK_F3))
    {
        auto& enemies = ObjectManager::Get().GetAllEnemies();
        int n = 0;
        for (auto& e : enemies)
            if (e.isAlive) { e.isAlive = 0; ++n; }
        Log("F3: Killed " + std::to_string(n) + " enemies.");
    }

    if (AEInputCheckTriggered(AEVK_F4))
    {
        textScreenMessage = "You Win";
        GameStateManager::Get().next = GS_WINLOSE;
    }

    if (AEInputCheckTriggered(AEVK_F5))
    {
        textScreenMessage = "You Lose";
        GameStateManager::Get().next = GS_WINLOSE;
    }
}

// ============================================================================
// Draw: stats overlay (top-left, screen-anchored)
// ============================================================================

void DebugManager::DrawOverlay(float camX, float camY) const
{
    const float winW = (float)AEGfxGetWindowWidth();
    const float winH = (float)AEGfxGetWindowHeight();

    MeshManager::Get().DrawSquare(
        camX - winW * 0.5f + OVL_PANEL_W * 0.5f,
        camY + winH * 0.5f - OVL_PANEL_H * 0.5f,
        OVL_PANEL_W, OVL_PANEL_H,
        8, 8, 8, 0.78f);

    if (g_FontMedium == -1) return;

    const Player& player = ObjectManager::Get().GetPlayer();
    const PhysicsManager& pm = PhysicsManager::Get();
    float y = OVL_START_Y;

    // Header
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "DEBUG  FPS: %.0f", (double)m_FpsCached);
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 0.2f, 1.f, 0.3f, 1.f);
        y -= OVL_LINE_H;
    }

    AEGfxPrint(g_FontMedium, "------------------------", OVL_TEXT_X, y,
        OVL_SCALE, 0.3f, 0.3f, 0.3f, 1.f);
    y -= OVL_LINE_H;

    // Player
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "Pos  (%.0f, %.0f)", (double)player.pos.x, (double)player.pos.y);
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 1, 1, 1, 1);
        y -= OVL_LINE_H;

        snprintf(buf, sizeof(buf), "Vel  (%.0f, %.0f)", (double)player.vel.x, (double)player.vel.y);
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 1, 1, 1, 1);
        y -= OVL_LINE_H;

        const char* wpn = "NONE";
        if (player.weapon == PlayerWeapon::MELEE) wpn = "MELEE";
        if (player.weapon == PlayerWeapon::GUN)   wpn = "GUN";

        snprintf(buf, sizeof(buf), "HP: %.0f  Gnd: %s  Wpn: %s",
            (double)player.hp, player.grounded ? "Y" : "N", wpn);
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 1, 1, 1, 1);
        y -= OVL_LINE_H;
    }

    // Camera & section
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "Cam  (%.0f, %.0f)  DbgCam: %s",
            (double)globalCam.x, (double)globalCam.y, globalCam.debugCam ? "ON" : "OFF");
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 1, 1, 1, 1);
        y -= OVL_LINE_H;

        int sec = EnvironmentManager::Get().GetCurrentSection();
        snprintf(buf, sizeof(buf), "Section: %d  (Level %d)", sec, sec + 1);
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, 1, 1, 1, 1);
        y -= OVL_LINE_H;
    }

    // God mode + hitbox status
    {
        float cr = m_GodMode ? 1.f : 0.5f;
        float cg = m_GodMode ? 0.8f : 0.5f;
        char buf[64];
        snprintf(buf, sizeof(buf), "GodMode: %s  Hitbox: %s",
            m_GodMode ? "ON" : "OFF", m_HitboxOn ? "ON" : "OFF");
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE, cr, cg, 0.1f, 1.f);
        y -= OVL_LINE_H;
    }

    // Physics -- live readout; tweaked values shown in amber
    AEGfxPrint(g_FontMedium, "-- Physics --", OVL_TEXT_X, y,
        OVL_SCALE, 0.3f, 0.3f, 0.3f, 1.f);
    y -= OVL_LINE_H;

    {
        char buf[80];

        bool lineASpd = Tweaked(pm.GetMoveSpeed(), DEF_SPEED);
        bool lineAGrv = Tweaked(pm.GetGravity(), DEF_GRAVITY);
        snprintf(buf, sizeof(buf), "spd=%.0f  grav=%.0f",
            (double)pm.GetMoveSpeed(), (double)pm.GetGravity());
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE,
            (lineASpd || lineAGrv) ? 1.f : 0.8f,
            (lineASpd || lineAGrv) ? 0.65f : 0.8f,
            0.15f, 1.f);
        y -= OVL_LINE_H;

        bool lineBJmp = Tweaked(pm.GetJumpForce(), DEF_JUMP);
        bool lineBTrm = Tweaked(pm.GetTerminalVel(), DEF_TERM);
        snprintf(buf, sizeof(buf), "jump=%.0f  term=%.0f",
            (double)pm.GetJumpForce(), (double)pm.GetTerminalVel());
        AEGfxPrint(g_FontMedium, buf, OVL_TEXT_X, y, OVL_SCALE,
            (lineBJmp || lineBTrm) ? 1.f : 0.8f,
            (lineBJmp || lineBTrm) ? 0.65f : 0.8f,
            0.15f, 1.f);
        y -= OVL_LINE_H;
    }

    // Footer (2 lines)
    AEGfxPrint(g_FontMedium,
        "F1=overlay  F2=console  F3=kill",
        OVL_TEXT_X, y, OVL_SCALE * 0.75f, 0.38f, 0.38f, 0.38f, 1.f);
    y -= OVL_LINE_H * 0.8f;
    AEGfxPrint(g_FontMedium,
        "F4=win  F5=lose  F6=hitbox",
        OVL_TEXT_X, y, OVL_SCALE * 0.75f, 0.38f, 0.38f, 0.38f, 1.f);
}

// ============================================================================
// Draw: console panel (flushed left, full width)
// ============================================================================

void DebugManager::DrawConsole(float camX, float camY)
{
    const float winW = (float)AEGfxGetWindowWidth();
    const float winH = (float)AEGfxGetWindowHeight();

    const float panelCentreY = camY - winH * 0.5f + CON_PANEL_H * 0.5f;
    MeshManager::Get().DrawSquare(camX, panelCentreY, winW, CON_PANEL_H, 8, 12, 8, 0.90f);
    MeshManager::Get().DrawSquare(camX, camY - winH * 0.5f + CON_PANEL_H - 2.f, winW, 4.f, 30, 200, 60, 1.f); //panel background

    if (g_FontMedium == -1) return;

    const float titleY = (-winH * 0.5f + CON_PANEL_H - 20.f) / (winH * 0.5f);
    AEGfxPrint(g_FontMedium,
        "DEVELOPER CONSOLE   Esc=close  Tab=complete  Up/Dn=history",
        CON_TEXT_X, titleY, CON_SCALE * 0.70f, 0.25f, 0.85f, 0.38f, 1.f);

    {
        int total = (int)m_Log.size();
        int start = std::max(0, total - CON_MAX_VISIBLE);
        float ly = CON_LOG_TOP_Y;
        for (int i = start; i < total; ++i)
        {
            float r = 0.82f, g2 = 0.82f, b = 0.82f;
            if (!m_Log[i].empty() && m_Log[i][0] == '>')
            {
                r = 0.35f; g2 = 1.f; b = 0.45f;
            }
            AEGfxPrint(g_FontMedium, m_Log[i].c_str(), CON_TEXT_X, ly, CON_SCALE, r, g2, b, 1.f);
            ly -= CON_LINE_H;
        }
    }

    const float inputWorldY = CON_INPUT_Y * (winH * 0.5f) + camY + 5.0f; //small offset cos it wasnt rendering right
    MeshManager::Get().DrawSquare(camX, inputWorldY, winW - 20.f, 36.f, 80, 80, 80, 1.f); //input box

    {
        std::string display = "> " + m_InputBuffer + (m_CursorBlink ? "|" : " ");
        AEGfxPrint(g_FontMedium, display.c_str(), CON_TEXT_X, CON_INPUT_Y, CON_SCALE, 0.2f, 1.f, 0.35f, 1.f);
    }

    if (!m_AutoHint.empty() && m_AutoHint.size() > m_InputBuffer.size())
    {
        static constexpr float k_CharW = 0.030f;
        float hintX = CON_TEXT_X + 2.f * k_CharW + (float)m_InputBuffer.size() * k_CharW - 0.03f;
        std::string suffix = m_AutoHint.substr(m_InputBuffer.size());
        AEGfxPrint(g_FontMedium, suffix.c_str(), hintX, CON_INPUT_Y, CON_SCALE,
            CON_HINT_ALPHA, CON_HINT_ALPHA, CON_HINT_ALPHA, 0.7f);
    }
}

// =========================================================================================
// Draw: world-space hitbox outlines + facing arrows (only used for debugging so it's here)
// =========================================================================================

// Hollow rectangle outline as 4 edge strips.
static void DrawOutline(float cx, float cy, float w, float h,
    int r, int g, int b, float alpha = 1.f, float thickness = 3.f)
{
    MeshManager::Get().DrawSquare(cx, cy + h * 0.5f - thickness * 0.5f, w, thickness, r, g, b, alpha); // top
    MeshManager::Get().DrawSquare(cx, cy - h * 0.5f + thickness * 0.5f, w, thickness, r, g, b, alpha); // bottom
    MeshManager::Get().DrawSquare(cx - w * 0.5f + thickness * 0.5f, cy, thickness, h, r, g, b, alpha); // left
    MeshManager::Get().DrawSquare(cx + w * 0.5f - thickness * 0.5f, cy, thickness, h, r, g, b, alpha); // right
}

//
//   facingRight=true:   box right edge --> ---->
//   facingRight=false:  <---- <-- box left edge
static void DrawArrow(float cx, float cy, float halfW, bool facingRight,
    int r, int g, int b, float alpha = 1.f)
{
    const float SHAFT = 32.f;  // total shaft length in world pixels
    const float HEAD = 12.f;  // arrowhead arm length
    const float THICK = 3.f;
    const float SPREAD = 10.f;  // arrowhead half-spread

    const float dir = facingRight ? 1.f : -1.f;
    // Tail of shaft sits flush with the box edge; shaft extends outward.
    const float tailX = cx + dir * halfW;
    const float shaftCX = tailX + dir * SHAFT * 0.5f; // centre of shaft square
    const float tipX = tailX + dir * SHAFT;         // arrowhead tip

    // Shaft
    MeshManager::Get().DrawSquare(shaftCX, cy, SHAFT, THICK, r, g, b, alpha);
    // Arrowhead arms (i didnt add rotation to this function but it's fine, just for debug)
    MeshManager::Get().DrawSquare(tipX - dir * HEAD * 0.5f, cy + SPREAD * 0.5f, HEAD, THICK, r, g, b, alpha);
    MeshManager::Get().DrawSquare(tipX - dir * HEAD * 0.5f, cy - SPREAD * 0.5f, HEAD, THICK, r, g, b, alpha);
}

static void DrawEntityLabel(float worldX, float worldY, float entityH,
    const char* text,
    float camX, float camY, float winW, float winH,
    float r, float g, float b)
{
    if (g_FontMedium == -1) return;
    float ndcX = (worldX - camX) / (winW * 0.5f);
    float ndcY = ((worldY + entityH * 0.5f + 10.f) - camY) / (winH * 0.5f);
    if (ndcX < -1.f || ndcX > 0.85f || ndcY < -0.98f || ndcY > 0.98f) return;
    AEGfxPrint(g_FontMedium, text, ndcX, ndcY, 0.40f, r, g, b, 1.f);
}

void DebugManager::DrawWorldOverlays(float camX, float camY) const
{
    const float winW = (float)AEGfxGetWindowWidth();
    const float winH = (float)AEGfxGetWindowHeight();

    if (m_GridOn) {
        EnvironmentManager::Get().GetSpatialGrid().DebugDrawGrid();
    }

    // Platform edit labels run independently of hitbox mode
    if (m_PlatformEditMode) {
        auto drawPlatformLabels = [&](const std::vector<Platform>& platforms, int level) {
            for (int i = 0; i < (int)platforms.size(); ++i) {
                const Platform& pf = platforms[i];
                // Highlight selected platform with white thick outline
                bool isSelected = (m_SelectedPlatform == &pf);
                int r = isSelected ? 255 : 180;
                int g = isSelected ? 255 : 180;
                int b = isSelected ? 255 :  60;
                float thickness = isSelected ? 5.f : 2.f;
                DrawOutline(pf.x, pf.y, pf.w, pf.h, r, g, b, 0.85f, thickness);

                char lbl[32];
                snprintf(lbl, sizeof(lbl), "L%d #%d", level, i);
                DrawEntityLabel(pf.x, pf.y, pf.h,
                    lbl, camX, camY, winW, winH,
                    r / 255.f, g / 255.f, b / 255.f);
            }
        };

        auto& env = EnvironmentManager::Get();
        drawPlatformLabels(env.GetLevel1Platforms(), 1);
        drawPlatformLabels(env.GetLevel2Platforms(), 2);
        drawPlatformLabels(env.GetLevel3Platforms(), 3);
        drawPlatformLabels(env.GetBossPlatforms(),   4);
    }

    if (!m_HitboxOn && !m_GridOn && !m_PlatformEditMode) return;
    if (!m_HitboxOn) return;

    // ---- Player (cyan) ------------------------------------------------------
    {
        const Player& p = ObjectManager::Get().GetPlayer();
        const float hw = p.width * 0.5f;
        //const float hh = p.height * 0.5f; no need anymore

        DrawOutline(p.pos.x, p.pos.y, p.width, p.height, 0, 220, 255, 0.90f);
        DrawArrow(p.pos.x, p.pos.y, hw, p.facingRight, 0, 220, 255, 0.95f);

        char lbl[48];
        snprintf(lbl, sizeof(lbl), "PLAYER  HP:%.0f", (double)p.hp);
        DrawEntityLabel(p.pos.x, p.pos.y, p.height,
            lbl, camX, camY, winW, winH, 0.2f, 0.88f, 1.f);

        for (const auto& b : p.bullets)
        {
            if (!b.active) continue;
            DrawOutline(b.pos.x, b.pos.y, b.width, b.height, 255, 240, 0, 0.90f, 2.f);
            DrawArrow(b.pos.x, b.pos.y, b.width * 0.5f, b.vel.x >= 0.f, 255, 240, 0, 0.85f);
        }

        // ---- Player slash hitbox (magenta outline, only when attacking) ----------
        if (p.isAttacking)
        {
            const float offset = 20.0f;
            float slashX = p.pos.x;
            float slashY = p.pos.y;
            float slashW = p.width;
            float slashH = p.height;

            switch (p.slashDirection) {
            case SlashDirection::HORIZONTAL:
                slashX += p.facingRight ? p.width * 0.5f + offset : -(p.width * 0.5f + offset);
                // width and height unchanged — horizontal box
                break;
            case SlashDirection::UP:
                slashY += p.height * 0.5f + offset;
                // sprite is rotated 90 degrees — swap width and height for the box
                slashW = p.height;
                slashH = p.width;
                break;
            case SlashDirection::DOWN:
                slashY -= p.height * 0.5f + offset;
                // sprite is rotated -90 degrees — swap width and height for the box
                slashW = p.height;
                slashH = p.width;
                break;
            }

            DrawOutline(slashX, slashY, slashW, slashH, 255, 0, 255, 0.90f);
            DrawEntityLabel(slashX, slashY, slashH,
                "SLASH", camX, camY, winW, winH, 1.f, 0.f, 1.f);
        }
    }

    // ---- Enemies (colour-coded by EnemyType) --------------------------------
    // enemy.h: EnemyType { Easy, Hard, Boss }; int isAlive; float hitPoints
    {
        const auto& enemies = ObjectManager::Get().GetAllEnemies();
        for (const auto& e : enemies)
        {
            if (!e.isAlive) continue;

            int cr = 255, cg = 80, cb = 80;
            const char* typeStr = "EASY";
            switch (e.type)
            {
            case EnemyType::Easy: cr = 255; cg = 80;  cb = 80;  typeStr = "EASY"; break;
            case EnemyType::Hard: cr = 255; cg = 165; cb = 0;   typeStr = "HARD"; break;
            case EnemyType::Boss: cr = 210; cg = 0;   cb = 255; typeStr = "BOSS"; break;
            }

            DrawOutline(e.pos.x, e.pos.y, (float)e.width, (float)e.height, cr, cg, cb, 0.90f);
            // Enemies use vel.x for facing; they don't have a sticky facingRight bool
            DrawArrow(e.pos.x, e.pos.y, e.width * 0.5f, e.vel.x >= 0.f, cr, cg, cb, 0.95f);

            char lbl[48];
            snprintf(lbl, sizeof(lbl), "%s  HP:%.0f", typeStr, (double)e.hitPoints);
            DrawEntityLabel(e.pos.x, e.pos.y, (float)e.height,
                lbl, camX, camY, winW, winH,
                (float)cr / 255.f, (float)cg / 255.f, (float)cb / 255.f);
        }
    }

    // ---- Enemy bullets (thin orange outline, velocity-based arrow) ----------
    {
        const auto& bullets = ObjectManager::Get().GetAllEnemyBullets();
        for (const auto& b : bullets)
        {
            if (!b.active) continue;
            DrawOutline(b.pos.x, b.pos.y, (float)b.width, (float)b.height, 255, 140, 0, 0.85f, 2.f);
            DrawArrow(b.pos.x, b.pos.y, b.width * 0.5f, b.direction, 255, 140, 0, 0.80f);
        }
    }

    // ---- Platform edit mode — level/index labels over every platform --------
    if (m_PlatformEditMode) {
        auto drawPlatformLabels = [&](const std::vector<Platform>& platforms, int level) {
            for (int i = 0; i < (int)platforms.size(); ++i) {
                const Platform& pf = platforms[i];
                // Highlight selected platform with white thick outline
                bool isSelected = (m_SelectedPlatform == &pf);
                int r = isSelected ? 255 : 180;
                int g = isSelected ? 255 : 180;
                int b = isSelected ? 255 :  60;
                float thickness = isSelected ? 5.f : 2.f;
                DrawOutline(pf.x, pf.y, pf.w, pf.h, r, g, b, 0.85f, thickness);

                char lbl[32];
                snprintf(lbl, sizeof(lbl), "L%d #%d", level, i);
                DrawEntityLabel(pf.x, pf.y, pf.h,
                    lbl, camX, camY, winW, winH,
                    r / 255.f, g / 255.f, b / 255.f);
            }
        };

        auto& env = EnvironmentManager::Get();
        drawPlatformLabels(env.GetLevel1Platforms(), 1);
        drawPlatformLabels(env.GetLevel2Platforms(), 2);
        drawPlatformLabels(env.GetLevel3Platforms(), 3);
        drawPlatformLabels(env.GetBossPlatforms(),   4);
    }
    // ---- Obstacles / spikes (yellow-green outline) --------------------------
    {
        auto drawObstacles = [&](const std::vector<PlatformObstacle>& obstacles) {
            for (const auto& obs : obstacles) {
                DrawOutline(obs.x, obs.y, obs.w, obs.h, 180, 255, 0, 0.85f);
                DrawEntityLabel(obs.x, obs.y, obs.h,
                    "SPIKE", camX, camY, winW, winH, 0.7f, 1.f, 0.f);
            }
        };
        auto& env = EnvironmentManager::Get();
        drawObstacles(env.GetLevel1Obstacles());
        drawObstacles(env.GetLevel2Obstacles());
        drawObstacles(env.GetLevel3Obstacles());
    }

    // ---- Lasers (red outline along actual laser direction) -------------------
    {
        auto drawLasers = [&](const std::vector<PlatformLaser>& lasers) {
            for (const auto& ls : lasers) {
                if (!ls.laserActive) continue;

                // Direction and perpendicular
                float dx = ls.x2 - ls.x1;
                float dy = ls.y2 - ls.y1;
                float len = sqrtf(dx * dx + dy * dy);
                if (len < 1e-6f) continue;

                float nx = -dy / len * (ls.w * 0.5f); // perpendicular offset
                float ny =  dx / len * (ls.w * 0.5f);

                // Four corners of the rotated rectangle
                float ax = ls.x1 + nx, ay = ls.y1 + ny;
                float bx = ls.x2 + nx, by = ls.y2 + ny;
                float cx = ls.x2 - nx, cy = ls.y2 - ny;
                float dx2 = ls.x1 - nx, dy2 = ls.y1 - ny;

                const float t = 3.f; // outline thickness
                MeshManager::Get().DrawLine(ax, ay, bx, by, t, 255, 30, 30, 0.90f);
                MeshManager::Get().DrawLine(bx, by, cx, cy, t, 255, 30, 30, 0.90f);
                MeshManager::Get().DrawLine(cx, cy, dx2, dy2, t, 255, 30, 30, 0.90f);
                MeshManager::Get().DrawLine(dx2, dy2, ax, ay, t, 255, 30, 30, 0.90f);

                // Label at midpoint
                float midX = (ls.x1 + ls.x2) * 0.5f;
                float midY = (ls.y1 + ls.y2) * 0.5f;
                DrawEntityLabel(midX, midY, ls.w,
                    "LASER", camX, camY, winW, winH, 1.f, 0.12f, 0.12f);
            }
        };
        auto& env = EnvironmentManager::Get();
        drawLasers(env.GetLevel2Lasers());
        drawLasers(env.GetLevel3Lasers());
    }
}

#endif // ENABLE_DEBUG_MANAGER

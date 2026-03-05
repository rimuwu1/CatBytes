/* Start Header ************************************************************************/
/*!
\file DebugManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 05/03/2026
\brief Singleton debug overlay, in-game console, and cheat manager.
       Toggle overlay : F1  (always)
       Toggle console : F2  (always; blocks game input while open)
       Toggle hitboxes: F6  (always; or 'hitbox' console command)
       Only active when ENABLE_DEBUG_MANAGER is defined (auto in _DEBUG).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#ifndef ENABLE_DEBUG_MANAGER
#  ifdef _DEBUG
#    define ENABLE_DEBUG_MANAGER
#  endif
#endif

#include <string>
#include <deque>
#include <vector>
#include <functional>
#include <unordered_map>

// ============================================================================
#ifdef ENABLE_DEBUG_MANAGER
// ============================================================================

struct DebugCommand
{
    std::string name;
    std::string usage;
    std::string description;
    std::function<void(const std::vector<std::string>& args)> handler;
};

class DebugManager
{
public:
    static DebugManager& Get()
    {
        static DebugManager instance;
        return instance;
    }

    // ---- Lifecycle ---------------------------------------------------------
    void Initialize();
    bool Update(float dt); // Returns true when the console is open (caller should skip normal input).
    void Draw(float camX, float camY);
    void DrawWorldOverlays(float camX, float camY) const;
    void Reset();

    // ---- Queries -----------------------------------------------------------
    bool IsOverlayVisible() const { return m_OverlayOn; }
    bool IsConsoleOpen()    const { return m_ConsoleOpen; }
    bool IsGodModeActive()  const { return m_GodMode; }
    bool IsHitboxVisible()  const { return m_HitboxOn; }

    // ---- Console log -------------------------------------------------------
    void Log(const std::string& msg);

    // ---- Command registration ----------------------------------------------
    void RegisterCommand(const std::string& name,
        const std::string& usage,
        const std::string& description,
        std::function<void(const std::vector<std::string>&)> handler);

private:
    DebugManager() = default;
    ~DebugManager() = default;
    DebugManager(const DebugManager&) = delete;
    DebugManager& operator=(const DebugManager&) = delete;

    void RegisterBuiltinCommands();

    void UpdateConsoleInput();
    void SubmitCommand();
    void RefreshAutoHint();
    std::vector<std::string> Tokenize(const std::string& s);

    void HandleHotkeys();

    void DrawOverlay(float camX, float camY) const;
    void DrawConsole(float camX, float camY);

    // ---- FPS ---------------------------------------------------------------
    int   m_FrameCount = 0;
    float m_FpsTimer = 0.f;
    float m_FpsCached = 0.f;

    // ---- Overlay / hitbox --------------------------------------------------
    bool  m_OverlayOn = false;   ///< F1 — stats panel
    bool  m_HitboxOn = false;   ///< F6 — world-space hitbox/arrow overlays
    bool  m_GodMode = false;
    bool  m_Initialized = false;

    // ---- Console -----------------------------------------------------------
    bool        m_ConsoleOpen = false;
    std::string m_InputBuffer;
    std::string m_AutoHint;
    bool        m_CursorBlink = true;
    float       m_BlinkTimer = 0.f;

    static constexpr float BLINK_RATE = 0.53f;
    static constexpr int   LOG_CAPACITY = 20;
    static constexpr int   HIST_CAPACITY = 32;

    std::deque<std::string> m_Log;
    std::deque<std::string> m_History;
    int                     m_HistoryIdx = -1;

    // ---- Commands ----------------------------------------------------------
    std::unordered_map<std::string, DebugCommand> m_Commands;
};

// ============================================================================
#else  // Release stub
// ============================================================================
class DebugManager
{
public:
    static DebugManager& Get() { static DebugManager i; return i; }
    void Initialize() {}
    bool Update(float) { return false; }
    void Draw(float, float) {}
    void DrawWorldOverlays(float, float) {}
    void Reset() {}
    bool IsOverlayVisible() const { return false; }
    bool IsConsoleOpen()    const { return false; }
    bool IsGodModeActive()  const { return false; }
    bool IsHitboxVisible()  const { return false; }
    void Log(const std::string&) {}
    void RegisterCommand(const std::string&, const std::string&,
        const std::string&,
        std::function<void(const std::vector<std::string>&)>) {
    }
};
#endif // ENABLE_DEBUG_MANAGER
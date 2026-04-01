/* Start Header ************************************************************************/
/*!
\file UIManager.cpp
\author Joash ng, joash.ng, 2502780
        Tse Xuan Qi Tristin, tse.x, 2503757
\par joash.ng@digipen.edu
     tse.x@digipen.edu
\date 03/03/2026
\brief This file implements functions to overlay & pause gamestates to render a popup menu/pause menu

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "UIManager.h"
#include "Fonts.h"
#include "MeshManager.h"
#include "GameStateManager.h"
#include "GameSaveManager.h"
#include "Controls.h"
#include "AudioManager.h"

// Layout constants - ALL positions in pixels (screen center = 0,0).
// DrawSquare  : pixel center (x,y) + pixel width/height
// AEGfxPrint  : NDC for left-baseline of first character

// -- Confirmation popup layout --
static const float PANEL_W = 500.0f;
static const float PANEL_H = 300.0f;
static const float PANEL_CX = 0.0f;
static const float PANEL_CY = 30.0f;
static const float TITLE_Y_PX = 130.0f;
static const float MSG_Y_PX = 50.0f;
static const float TEXT_LEFT_PX = -(PANEL_W * 0.5f) + 20.0f;
static const float BTN_W = 120.0f;
static const float BTN_H = 50.0f;
static const float BTN_Y_PX = -80.0f;
static const float BTN_YES_X_PX = -100.0f;
static const float BTN_NO_X_PX = 100.0f;

// -- Pause menu layout (NDC) --
static const float PAUSE_BTN_X = -0.4f;    // left edge of text
static const float PAUSE_BTN_W = 1.2f;    // wide enough to cover longest label
static const float PAUSE_BTN_H = 0.08f;
static const float PAUSE_Y_OFFSET = 0.03f;
static const float PAUSE_START_Y = 0.25f;
static const float PAUSE_SPACING = 0.15f;
static const float PAUSE_PANEL_W_PX = 750.0f;
static const float PAUSE_PANEL_H_PX = 600.0f;

static AEAudio s_PauseHoverSound{};
static AEAudio s_PauseClickSound{};
static bool    s_PauseSoundsLoaded = false;

bool g_newGame = false;

static float PxToNdcX(float px, float winW) { return px / (winW * 0.5f); }
static float PxToNdcY(float py, float winH) { return py / (winH * 0.5f); }
static float PauseBtnY(int idx) { return PAUSE_START_Y - (idx - 1) * PAUSE_SPACING; }

// ----------------------------------------------------------------------------
// ShowConfirmation
// ----------------------------------------------------------------------------
void UIManager::ShowConfirmation(const std::string& title,
    const std::string& message,
    std::function<void()> onConfirm,
    std::function<void()> onCancel)
{
    if (m_PopupActive) {
        std::cout << "UIManager: popup already active, ignoring.\n";
        return;
    }
    m_Popup.title = title;
    m_Popup.message = message;
    m_Popup.onConfirm = onConfirm;
    m_Popup.onCancel = onCancel;
    m_PopupActive = true;
}

// ----------------------------------------------------------------------------
// ShowPause / HidePause
// ----------------------------------------------------------------------------
void UIManager::ShowPause()
{
    if (m_PauseActive) return;  // already open: ignore re-entry
    if (!s_PauseSoundsLoaded) {
        s_PauseHoverSound = AudioManager::Get().GetAudio("hover_button");
        s_PauseClickSound = AudioManager::Get().GetAudio("click_button");
        s_PauseSoundsLoaded = true;
    }
    m_PauseActive = true;
    m_PauseHovered = 0;
    m_PausePrevHov = 0;
}


// ----------------------------------------------------------------------------
// Reset
// ----------------------------------------------------------------------------
void UIManager::Reset()
{
    m_PopupActive = false;
    m_Popup.title = "";
    m_Popup.message = "";
    m_Popup.onConfirm = nullptr;
    m_Popup.onCancel = nullptr;
    m_PauseActive = false;
    m_PauseHovered = 0;
    m_PausePrevHov = 0;
}

// ----------------------------------------------------------------------------
// Update -- confirmation takes priority over pause
// ----------------------------------------------------------------------------
bool UIManager::Update(float camX, float camY)
{
    if (!IsActive()) return false;
    bool consumed = false;
    if (m_PopupActive) UpdateConfirmation(camX, camY, consumed);
    else if (m_PauseActive) UpdatePause(camX, camY, consumed);
    return true;  // modal: always consume input when any overlay is up
}

// ----------------------------------------------------------------------------
// Draw -- pause drawn first, confirmation drawn on top
// ----------------------------------------------------------------------------
void UIManager::Draw(float camX, float camY)
{
    if (!IsActive()) return;
    if (m_PauseActive) DrawPause(camX, camY);
    if (m_PopupActive) DrawConfirmation(camX, camY);
}

// ============================================================================
// Confirmation internals
// ============================================================================
void UIManager::UpdateConfirmation(float camX, float camY, bool& consumed)
{
    f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();
    float btnWNdc = BTN_W / (winW * 0.5f);
    float btnHNdc = BTN_H / (winH * 0.5f);
    float btnYNdc = PxToNdcY(BTN_Y_PX, winH);
    float btnYesX = PxToNdcX(BTN_YES_X_PX, winW);
    float btnNoX = PxToNdcX(BTN_NO_X_PX, winW);

    if (AEInputCheckTriggered(AEVK_LBUTTON)) {
        if (IsMouseOverButton(btnYesX, btnYNdc, btnWNdc, btnHNdc, camX, camY)) {
            if (m_Popup.onConfirm) m_Popup.onConfirm();
            m_PopupActive = false; consumed = true; return;
        }
        if (IsMouseOverButton(btnNoX, btnYNdc, btnWNdc, btnHNdc, camX, camY)) {
            if (m_Popup.onCancel) m_Popup.onCancel();
            m_PopupActive = false; consumed = true; return;
        }
    }
    consumed = true;
}

void UIManager::DrawConfirmation(float camX, float camY)
{
    f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();

    // Extra dim layer on top of pause (if both active) or game world
    MeshManager::Get().DrawSquare(camX, camY, winW, winH, 0, 0, 0, 0.55f);
    MeshManager::Get().DrawSquare(camX + PANEL_CX, camY + PANEL_CY, PANEL_W, PANEL_H, 30, 30, 30);

    if (g_FontLarge != -1)
        FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), m_Popup.title.c_str(),
            camX, PxToNdcY(TITLE_Y_PX, winH),
            0.6f, 1, 1, 1, 1);

    if (g_FontMedium != -1)
        FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(), m_Popup.message.c_str(),
            camX, PxToNdcY(MSG_Y_PX, winH),
            0.5f, 1, 1, 1, 1);

    float btnWNdc = BTN_W / (winW * 0.5f);
    float btnHNdc = BTN_H / (winH * 0.5f);
    float btnYNdc = PxToNdcY(BTN_Y_PX, winH);
    float btnYesX = PxToNdcX(BTN_YES_X_PX, winW);
    float btnNoX = PxToNdcX(BTN_NO_X_PX, winW);
    float textOffY = -btnHNdc * 0.2f;

    { // YES
        bool hover = IsMouseOverButton(btnYesX, btnYNdc, btnWNdc, btnHNdc, camX, camY);
        int s = hover ? 180 : 100;
        MeshManager::Get().DrawSquare(camX + BTN_YES_X_PX, camY + BTN_Y_PX, BTN_W, BTN_H, s, s, s);
        if (g_FontMedium != -1) {
            float hw = 3.f * 0.04f * 0.5f * 0.5f;
            FontManager::Get().Print(FontManager::Get().GetMediumFont(), "YES", btnYesX - hw, btnYNdc + textOffY, 0.5f, 1, 1, 1, 1);
        }
    }
    { // NO
        bool hover = IsMouseOverButton(btnNoX, btnYNdc, btnWNdc, btnHNdc, camX, camY);
        int s = hover ? 180 : 100;
        MeshManager::Get().DrawSquare(camX + BTN_NO_X_PX, camY + BTN_Y_PX, BTN_W, BTN_H, s, s, s);
        if (g_FontMedium != -1) {
            float hw = 2.f * 0.04f * 0.5f * 0.5f;
            FontManager::Get().Print(FontManager::Get().GetMediumFont(), "NO", btnNoX - hw, btnYNdc + textOffY, 0.5f, 1, 1, 1, 1);
        }
    }
}

// ============================================================================
// Pause menu internals
// ============================================================================
void UIManager::UpdatePause(float camX, float camY, bool& consumed)
{
    (void)camX; (void)camY;
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();
    float ndcX = ((f32)mx / winW) * 2.f - 1.f;
    float ndcY = 1.f - ((f32)my / winH) * 2.f;

    // ESC resumes -- checked first so Input_Handle's ShowPause() call on the same
    // frame doesn't fight with this (ShowPause guards against re-entry when active)
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        m_PauseActive = false;
        consumed = true;
        return;
    }

    // Button detection: left-edge X range + Y offset to text center
    m_PauseHovered = 0;
    for (int i = 1; i <= 5; ++i) {
        float detectY = PauseBtnY(i) + PAUSE_Y_OFFSET;
        if (ndcX >= PAUSE_BTN_X && ndcX <= PAUSE_BTN_X + PAUSE_BTN_W &&
            ndcY >= detectY - PAUSE_BTN_H * 0.5f && ndcY <= detectY + PAUSE_BTN_H * 0.5f)
        {
            m_PauseHovered = i;
            break;
        }
    }

    if (m_PauseHovered != 0 && m_PauseHovered != m_PausePrevHov)
        AudioManager::Get().PlayAudio(s_PauseHoverSound, false);
    m_PausePrevHov = m_PauseHovered;

    if (m_PauseHovered != 0 && AEInputCheckTriggered(AEVK_LBUTTON)) {
        AudioManager::Get().PlayAudio(s_PauseClickSound, false);
        switch (m_PauseHovered) {
        case 1: // Resume
            m_PauseActive = false;
            break;
        case 2: // Controls
            m_PauseActive = false;
            g_FromPause = true;
            GameStateManager::Get().next = GS_CONTROLS;
            break;
        case 3: // Restart -- confirm first
            m_PauseActive = false;
            ShowConfirmation("Restart Game?", "All progress will be lost!",
                []() { g_newGame = true; GameSaveManager::ResetSave(); },
                []() {});
            break;
        case 4: // Main Menu -- confirm first
            m_PauseActive = false;
            ShowConfirmation("Exit to Main Menu?", "Unsaved progress will be lost!",
                []() { GameStateManager::Get().next = GS_MAINMENU; },
                []() {});
            break;
        case 5: // Quit -- confirm first
            m_PauseActive = false;
            ShowConfirmation("Quit Game?", "Are you sure you want to quit?",
                []() { GameStateManager::Get().next = GS_QUIT; },
                []() {});
            break;
        }
    }
    consumed = true;
}

void UIManager::DrawPause(float camX, float camY)
{
    f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();

    // Dim overlay over game world
    MeshManager::Get().DrawSquare(camX, camY, winW, winH, 0, 0, 0, 0.55f);
    // Panel
    MeshManager::Get().DrawSquare(camX, camY, PAUSE_PANEL_W_PX, PAUSE_PANEL_H_PX, 30, 30, 30);

    if (g_FontLarge == -1 || g_FontMedium == -1) return;

    // Title -- centered
    FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "PAUSED", camX, PauseBtnY(0) + 0.10f, 0.8f, 1, 1, 1, 1);

    const char* labels[] = { "", "RESUME GAME", "CONTROLS", "RESTART", "EXIT TO MAIN MENU", "QUIT GAME" };
    for (int i = 1; i <= 5; ++i) {
        float bright = (m_PauseHovered == i) ? 1.0f : 0.6f;
        FontManager::Get().Print(FontManager::Get().GetMediumFont(), labels[i], PAUSE_BTN_X, PauseBtnY(i), 1.0f, bright, bright, bright, 1.0f);
    }
}

// ============================================================================
// Shared NDC hit-test helper
// ============================================================================
bool UIManager::IsMouseOverButton(float btnX, float btnY,
    float btnWidth, float btnHeight,
    float, float) const
{
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);
    f32 winW = (f32)AEGfxGetWindowWidth();
    f32 winH = (f32)AEGfxGetWindowHeight();
    f32 ndcX = ((f32)mx / winW) * 2.f - 1.f;
    f32 ndcY = 1.f - ((f32)my / winH) * 2.f;
    return (ndcX >= btnX - btnWidth * 0.5f && ndcX <= btnX + btnWidth * 0.5f &&
        ndcY >= btnY - btnHeight * 0.5f && ndcY <= btnY + btnHeight * 0.5f);
}

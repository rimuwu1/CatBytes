/* Start Header ************************************************************************/
/*!
\file Controls.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "Controls.h"
#include "GameStateManager.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "AEEngine.h"
#include "AudioManager.h"
#include "Fonts.h"

static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};
static int g_PreviousHoveredButton = -1;

//State
enum ControlTab
{
    TAB_KEYS = 0,
    TAB_SETTINGS
};

static ControlTab g_CurrentTab = TAB_KEYS;

static int g_HoveredButton = -1;


// textures, temp
static AEGfxTexture* g_BG = nullptr;
static AEGfxTexture* g_SettingsBtn = nullptr;
static AEGfxTexture* g_KeysBtn = nullptr;
static AEGfxTexture* g_SettingsPanel = nullptr;

// button layout
const float BTN_X = -0.85f;
const float BTN_WIDTH = 0.2f;
const float BTN_HEIGHT = 0.15f;

const float BTN_KEYS_Y = 0.2f;
const float BTN_SETTINGS_Y = 0.0f;

//check for hovering
static bool IsMouseOver(float mx, float my, float x, float y)
{
    return (mx >= x && mx <= x + BTN_WIDTH &&
        my >= y - BTN_HEIGHT * 0.5f &&
        my <= y + BTN_HEIGHT * 0.5f);
}

void Controls_Load()
{

    //gonna change later
    g_BG = TextureManager::Get().LoadTexture("Assets/Images/ControlPage.png");
    g_SettingsBtn = TextureManager::Get().LoadTexture("Assets/Images/settings.png");
    g_KeysBtn = TextureManager::Get().LoadTexture("Assets/Images/Keys.png");
    g_SettingsPanel = TextureManager::Get().LoadTexture("Assets/Images/settingstest.png");

    std::cout << "Controls:Load" << std::endl;
}

void Controls_Initialize()
{
    std::cout << "Controls:Initialize" << std::endl;
    g_HoverSound = AudioManager::Get().GetAudio("hover_button");
    g_ClickSound = AudioManager::Get().GetAudio("click_button");
    g_PreviousHoveredButton = -1;
    g_CurrentTab = TAB_KEYS;


    std::cout << "Controls:Load" << std::endl;
}

void Controls_Update()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        GameStateManager::Get().next = GS_MAINMENU;
    }

    // mouse position
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    float nx = (mx / w) * 2.0f - 1.0f;
    float ny = 1.0f - (my / h) * 2.0f;

    g_HoveredButton = -1;

    if (IsMouseOver(nx, ny, BTN_X, BTN_KEYS_Y))
        g_HoveredButton = TAB_KEYS;
    else if (IsMouseOver(nx, ny, BTN_X, BTN_SETTINGS_Y))
        g_HoveredButton = TAB_SETTINGS;

    //hover
    if (g_HoveredButton != -1 && g_HoveredButton != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }
    g_PreviousHoveredButton = g_HoveredButton;

    // click
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (g_HoveredButton == TAB_KEYS)
        {
            g_CurrentTab = TAB_KEYS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == TAB_SETTINGS)
        {
            g_CurrentTab = TAB_SETTINGS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
    }

}
//temp
void DrawKeysPage()
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float startX = -0.2f;
    float y = 0.4f;
    float spacing = 0.07f;

    auto Print = [&](const char* text)
        {
            AEGfxPrint(g_FontSmall, text, startX, y, 1.0f, 1, 1, 1, 1);
            y -= spacing;
        };

    // Navigation
    AEGfxPrint(g_FontMedium, "NAVIGATION", startX, y, 1.2f, 1, 1, 1, 1);
    y -= spacing;

    Print("WASD - Move");
    Print("ESC - Pause");
    Print("Q - Quit");
    Print("E - interation with platform/computer");

    y -= spacing;

    // Abilities
    AEGfxPrint(g_FontMedium, "ABILITIES", startX, y, 1.2f, 1, 1, 1, 1);
    y -= spacing;

    Print("Space - Jump");
    Print("1/2/3 - Weapon Switch");
    Print("4/5/6 - Inventory");
    Print("Right Click - Dash");
}



void Controls_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    // background
    if (g_BG)
    {
        MeshManager::Get().DrawTexturedSquare(g_BG, 0, 0, w, h, 1.0f);
    }

    float halfH = h / 2.0f;
    float btnX = (BTN_X * (w / 2.0f)) + 80.0f;

    // draw KEYS button
    if (g_KeysBtn)
    {
        float y = BTN_KEYS_Y * halfH;
        float scale = (g_HoveredButton == TAB_KEYS) ? 1.1f : 1.0f;

        MeshManager::Get().DrawTexturedSquare(
            g_KeysBtn, btnX, y,
            120.0f * scale, 60.0f * scale, 1.0f
        );
    }

    // draw SETTINGS button
    if (g_SettingsBtn)
    {
        float y = BTN_SETTINGS_Y * halfH;
        float scale = (g_HoveredButton == TAB_SETTINGS) ? 1.1f : 1.0f;

        MeshManager::Get().DrawTexturedSquare(
            g_SettingsBtn, btnX, y,
            120.0f * scale, 60.0f * scale, 1.0f
        );
    }

    // draw page
    if (g_CurrentTab == TAB_KEYS)
    {
        DrawKeysPage();
    }
    else if (g_CurrentTab == TAB_SETTINGS)
    {
        if (g_SettingsPanel)
        {
            MeshManager::Get().DrawTexturedSquare(
                g_SettingsPanel,
                200.0f,// move right side
                0.0f,
                400.0f,
                300.0f,
                1.0f
            );
        }
    }

    AESysFrameEnd();

}


void Controls_Free()
{
    std::cout << "Controls:Free" << std::endl;
}

void Controls_Unload()
{
    //temp
    TextureManager::Get().UnloadTexture("Assets/Images/ControlPage.png");
    TextureManager::Get().UnloadTexture("Assets/Images/settings.png");
    TextureManager::Get().UnloadTexture("Assets/Images/Keys.png");
    TextureManager::Get().UnloadTexture("Assets/Images/settingstest.png");

    g_BG = nullptr;
    g_SettingsBtn = nullptr;
    g_KeysBtn = nullptr;
    g_SettingsPanel = nullptr;

    std::cout << "Controls:Unload" << std::endl;
}
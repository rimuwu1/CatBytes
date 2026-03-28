/* Start Header ************************************************************************/
/*!
\file Controls.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par tse.x@digipen.edu
     kerwinjiajie.wong@digipen.edu
\date January, 24, 2026
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
    TAB_INSTRUCTIONS,
    TAB_SETTINGS
};

static ControlTab g_CurrentTab = TAB_KEYS;

static int g_HoveredButton = -1;


// textures, temp
static AEGfxTexture* g_BG = nullptr;
static AEGfxTexture* g_SettingsBtn = nullptr;
static AEGfxTexture* g_KeysBtn = nullptr;
static AEGfxTexture* g_InstructionsBtn = nullptr;
static AEGfxTexture* g_SettingsPanel = nullptr;

// textures for instructions page
static AEGfxTexture* g_ImgMelee = nullptr;
static AEGfxTexture* g_ImgRanged = nullptr;
static AEGfxTexture* g_ImgCheckpoint = nullptr;
static AEGfxTexture* g_ImgSwitch = nullptr;
static AEGfxTexture* g_ImgComputer = nullptr;
static AEGfxTexture* g_ImgBossRoom = nullptr;
static AEGfxTexture* g_ImgHpRecovery = nullptr;
static AEGfxTexture* g_ImgShield = nullptr;
static AEGfxTexture* g_ImgDash = nullptr;

// button layout
const float BTN_X = -0.85f;
const float BTN_WIDTH = 0.2f;
const float BTN_HEIGHT = 0.15f;

const float BTN_KEYS_Y = 0.22f;
const float BTN_INSTRUCTIONS_Y = 0.05f;
const float BTN_SETTINGS_Y =-0.12f;

// instruction card layout
const float CARD_LEFT = -0.62f;
const float CARD_TOP_Y = 0.68f;
const float CARD_W = 0.56f;
const float CARD_H = 0.52f;
const float CARD_GAP_X = 0.04f;
const float CARD_GAP_Y = 0.05f;
const float IMG_SIZE = 0.55f;

struct InstructionCard
{
    AEGfxTexture** img;
    const char* title;
    const char* desc;
};

static InstructionCard g_CombatCards[] =
{
    { &g_ImgMelee, "Melee Weapon", "Does more damage" },
    { &g_ImgRanged, "Ranged Weapon", "Does less damage" }
};

static InstructionCard g_InteractionCards[] =
{
    { &g_ImgCheckpoint, "Checkpoint", "Saves your progress" },
    { &g_ImgSwitch, "Toggle Switch", "Unlocks platforms & walls" },
    { &g_ImgComputer, "Computer", "Kills enemies with lasers" },
    { &g_ImgBossRoom, "Boss Room", "Moves to the boss stage" }
};

static InstructionCard g_BuffCards[] =
{
    { &g_ImgHpRecovery, "HP Recovery", "Restores full health" },
    { &g_ImgShield, "Shield", "Blocks damage for 10 seconds" },
    { &g_ImgDash, "Dash", "Gain dash uses" }
};

static constexpr int g_WeaponCount = static_cast<int>(sizeof(g_CombatCards) / sizeof(g_CombatCards[0]));
static constexpr int g_InteractionCount = static_cast<int>(sizeof(g_InteractionCards) / sizeof(g_InteractionCards[0]));
static constexpr int g_BuffCount = static_cast<int>(sizeof(g_BuffCards) / sizeof(g_BuffCards[0]));

//check for hovering
static bool IsMouseOver(float mx, float my, float x, float y)
{
    return (mx >= x && mx <= x + BTN_WIDTH &&
        my >= y - BTN_HEIGHT * 0.5f &&
        my <= y + BTN_HEIGHT * 0.5f);
}

// for drawing instruction cards
static void DrawRect(
    float cx, float cy, float pw, float ph, 
    float r, float g, float b, float a
)
{
    MeshManager::Get().DrawSquare(
        cx, cy, pw, ph,
        static_cast<int>(r * 255),
        static_cast<int>(g * 255),
        static_cast<int>(b * 255),
        a
    );
}

static void DrawInstructionCard(const InstructionCard& card, float cx, float cy, float cardW, float cardH, float halfW, float halfH)
{
    DrawRect(cx, cy, cardW, cardH, 0.0f, 0.06f, 0.20f, 0.55f);

    float imgH = cardH * IMG_SIZE;
    float imgCY = cy + (cardH - imgH) * 0.5f;

    if (*card.img)
    {
        MeshManager::Get().DrawTexturedSquare(
            *card.img, 
            cx, imgCY, 
            cardW - 4.0f, imgH - 4.0f,
            1.0f
        );
    }
    else
    {
        DrawRect(cx, imgCY, cardW - 4.0f, imgH - 4.0f, 0.0f, 0.10f, 0.30f, 0.60f);
    }

    // gap between img and text desc
    float divY = imgCY - imgH * 0.5f;
    DrawRect(cx, divY, cardW - 4.0f, 1.5f, 0.0f, 0.45f, 0.70f, 0.30f);

    // text part
    float textSectionH = cardH * (1.0f - IMG_SIZE);
    float textTopCY = cy - cardH * 0.5f + textSectionH;
    float titleX = (cx - cardW * 0.5f + 10.0f) / halfW;
    float titleY = (textTopCY - textSectionH * 0.28f) / halfH;
    float descY = titleY - 0.065f;

    // title
    FontManager::Get().Print(
        FontManager::Get().GetSmallFont(),
        card.title,
        titleX, titleY,
        1.0f, 0.70f, 0.90f, 1.0f, 1.0f
    );

    // text desc
    FontManager::Get().Print(
        FontManager::Get().GetSmallFont(),
        card.desc,
        titleX, descY,
        0.80f, 0.38f, 0.58f, 0.72f, 1.0f
    );
}

void Controls_Load()
{

    //gonna change later
    g_BG = TextureManager::Get().LoadTexture("Assets/Images/ControlPage.png");
    g_SettingsBtn = TextureManager::Get().LoadTexture("Assets/Images/settings.png");
    g_KeysBtn = TextureManager::Get().LoadTexture("Assets/Images/Keys.png");
    g_InstructionsBtn = TextureManager::Get().LoadTexture("Assets/Images/Keys.png");
    g_SettingsPanel = TextureManager::Get().LoadTexture("Assets/Images/settingstest.png");

    // instructions page textures
    g_ImgMelee = TextureManager::Get().LoadTexture("Assets/Images/controls_melee.png");
    g_ImgRanged = TextureManager::Get().LoadTexture("Assets/Images/controls_ranged.png");
    g_ImgCheckpoint = TextureManager::Get().LoadTexture("Assets/Images/controls_checkpoint.png");
    g_ImgSwitch = TextureManager::Get().LoadTexture("Assets/Images/controls_switch.png");
    g_ImgComputer = TextureManager::Get().LoadTexture("Assets/Images/controls_computer.png");
    g_ImgBossRoom = TextureManager::Get().LoadTexture("Assets/Images/controls_bossdoor.png");
    g_ImgHpRecovery = TextureManager::Get().LoadTexture("Assets/Images/controls_full_hp.png");
    g_ImgShield = TextureManager::Get().LoadTexture("Assets/Images/controls_shield.png");
    g_ImgDash = TextureManager::Get().LoadTexture("Assets/Images/controls_dash.png");

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
    else if (IsMouseOver(nx, ny, BTN_X, BTN_INSTRUCTIONS_Y))
        g_HoveredButton = TAB_INSTRUCTIONS;
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
        else if (g_HoveredButton == TAB_INSTRUCTIONS)
        {
            g_CurrentTab = TAB_INSTRUCTIONS;
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
            FontManager::Get().Print(FontManager::Get().GetSmallFont(), text, startX, y, 1.0f, 1, 1, 1, 1);
            y -= spacing;
        };

    // Navigation
    FontManager::Get().Print(FontManager::Get().GetMediumFont(), "NAVIGATION", startX, y, 1.2f, 1, 1, 1, 1);
    y -= spacing;

    Print("WASD - Move");
    Print("ESC - Pause");
    Print("Q - Quit");
    Print("E - interaction with platform/computer");

    y -= spacing;

    // Abilities
    FontManager::Get().Print(FontManager::Get().GetMediumFont(), "ABILITIES", startX, y, 1.2f, 1, 1, 1, 1);
    y -= spacing;

    Print("Space - Jump");
    Print("1/2/3 - Weapon Switch");
    Print("4/5/6 - Inventory");
    Print("Right Click - Dash");
}

// draw instructions page
static void DrawInstructionsPage(float halfW, float halfH)
{
    float cardW = 300.0f;
    float cardH = 120.0f;
    float gapX = 30.0f;
    float gapY = 18.0f;

    float leftX = -140.0f;
    float midX = leftX + cardW + gapX;
    float rightX = midX + cardW + gapX;

    float y = 340.0f;

    auto DrawSectionTitle = [&](const char* text, float nx, float ny)
    {
        FontManager::Get().Print(
            FontManager::Get().GetMediumFont(),
            text, nx, ny,
            1.0f, 1, 1, 1, 1
        );
    };

    // ---- Combat / Weapon Section ---- //
    DrawSectionTitle("COMBAT / WEAPONS", -0.37f, y / halfH);
    y -= 70.0f;
    DrawInstructionCard(g_CombatCards[0], leftX, y, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_CombatCards[1], midX, y, cardW, cardH, halfW, halfH);

    // ---- Interaction Section ---- //
    y -= 145.0f;
    DrawSectionTitle("INTERACTION", -0.37f, y / halfH);

    y -= 70.0f;
    DrawInstructionCard(g_InteractionCards[0], leftX, y, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_InteractionCards[1], midX, y, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_InteractionCards[2], leftX, y - cardH - gapY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_InteractionCards[3], midX, y - cardH - gapY, cardW, cardH, halfW, halfH);

    // ---- Buff Section ---- //
    float buffY = y - 2.0f * (cardH + gapY) - 35.0f;
    DrawSectionTitle("BUFFS", -0.37f, buffY / halfH);

    buffY -= 70.0f;

    DrawInstructionCard(g_BuffCards[0], leftX, buffY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_BuffCards[1], midX, buffY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_BuffCards[2], rightX, buffY, cardW, cardH, halfW, halfH);
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

    float halfW = w / 2.0f;
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

    // draw INSTRUCTIONS button
    if (g_InstructionsBtn)
    {
        float y = BTN_INSTRUCTIONS_Y * halfH;
        float scale = (g_HoveredButton == TAB_INSTRUCTIONS) ? 1.1f : 1.0f;

        MeshManager::Get().DrawTexturedSquare(
            g_InstructionsBtn, btnX, y,
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
    else if (g_CurrentTab == TAB_INSTRUCTIONS)
    {
        DrawInstructionsPage(halfW, halfH);
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
    
    // unload instruction page textures
    TextureManager::Get().UnloadTexture("Assets/Images/controls_melee.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_ranged.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_checkpoint.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_switch.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_computer.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_bossdoor.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_full_hp.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_shield.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_dash.png");

    g_BG = nullptr;
    g_SettingsBtn = nullptr;
    g_KeysBtn = nullptr;
    g_InstructionsBtn = nullptr;
    g_SettingsPanel = nullptr;

    // instructions page
    g_ImgMelee = nullptr;
    g_ImgRanged = nullptr;
    g_ImgCheckpoint = nullptr;
    g_ImgSwitch = nullptr;
    g_ImgComputer = nullptr;
    g_ImgBossRoom = nullptr;
    g_ImgHpRecovery = nullptr;
    g_ImgShield = nullptr;
    g_ImgDash = nullptr;

    std::cout << "Controls:Unload" << std::endl;
}
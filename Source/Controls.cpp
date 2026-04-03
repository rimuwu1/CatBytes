/* Start Header ************************************************************************/
/*!
\file Controls.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par tse.x@digipen.edu
     kerwinjiajie.wong@digipen.edu
\date January, 24, 2026
\brief Implements the Controls menu and in-game controls overlay, including
tab switching, button hover/click handling, instruction card rendering,
and audio settings sliders for music and SFX volume.

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
#include "UIManager.h"
#include "AEEngine.h"
#include "AudioManager.h"
#include "Fonts.h"
#include <memory>
#include "SpriteSheet.h"
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

// Controls menu state and button sprite sheet configuration
bool g_FromPause = false;
static std::unique_ptr<SpriteSheet> g_ControlButtonSheet = nullptr;
static float g_ControlButtonWidth = 120.0f;
static float g_ControlButtonHeight = 60.0f;

// Audio feedback and settings slider state
static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};
static int g_PreviousHoveredButton = -1;
static float g_MusicSlider = 1.0f;
static float g_SFXSlider = 1.0f;
static bool g_DragMusic = false;
static bool g_DragSFX = false;

// Clamps a floating-point value to the range [0.0f, 1.0f]
static float Clamp01(float v) noexcept
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Tabs available in the controls menu
enum ControlTab
{
    TAB_KEYS = 0,
    TAB_INSTRUCTIONS,
    TAB_SETTINGS
};

// Loads the controls menu configuration from GameConfig.json
// Returns an empty JSON object if the file cannot be opened or parsed
static rapidjson::Document LoadConfig() noexcept
{
    rapidjson::Document doc;

    std::ifstream ifs("Assets/Data/GameConfig.json");

    if (!ifs.is_open())
    {
        doc.SetObject();
        return doc;
    }

    rapidjson::IStreamWrapper isw(ifs);
    doc.ParseStream(isw);

    if (!doc.IsObject())
        doc.SetObject();

    return doc;
}

// Sprite-sheet frame indices for each controls-menu button state
enum ControlButtonFrame
{
    FRAME_KEYS = 0,
    FRAME_INSTRUCTIONS = 1,
    FRAME_SETTINGS = 2,
    FRAME_BACK = 3,

    FRAME_KEYS_PRESSED = 4,
    FRAME_INSTRUCTIONS_PRESSED = 5,
    FRAME_SETTINGS_PRESSED = 6
};

// Logical button IDs used for hover/click handling
enum ControlButton
{
    CONTROLBTN_NONE = -1,
    CONTROLBTN_KEYS = 0,
    CONTROLBTN_INSTRUCTIONS,
    CONTROLBTN_SETTINGS,
    CONTROLBTN_BACK
};

// Returns true if the mouse position (in normalized device coordinates)
// lies within a rectangle centered at (cx, cy)
static bool IsMouseOverRect(float mx, float my, float cx, float cy, float w, float h) noexcept
{
    return (fabs(mx - cx) <= w * 0.5f &&
        fabs(my - cy) <= h * 0.5f);
}

// Current UI tab and currently hovered button
static ControlTab g_CurrentTab = TAB_KEYS;
static int g_HoveredButton = CONTROLBTN_NONE;


// Menu textures
static AEGfxTexture* g_BG = nullptr;
static AEGfxTexture* g_SettingsPanel = nullptr;
static AEGfxTexture* g_Background = nullptr;

// textures for instructions page
static AEGfxTexture* g_ImgMelee = nullptr;
static AEGfxTexture* g_ImgRanged = nullptr;
static AEGfxTexture* g_ImgCheckpoint = nullptr;
static AEGfxTexture* g_ImgSwitch = nullptr;
static AEGfxTexture* g_ImgSpike = nullptr;
static AEGfxTexture* g_ImgComputer = nullptr;
static AEGfxTexture* g_ImgBossRoom = nullptr;
static AEGfxTexture* g_ImgHpRecovery = nullptr;
static AEGfxTexture* g_ImgShield = nullptr;
static AEGfxTexture* g_ImgDash = nullptr;

// button layout
const float BTN_X = -0.85f;
const float BTN_WIDTH = 0.2f;
const float BTN_HEIGHT = 0.15f;

const float BTN_KEYS_Y = 0.30f;
const float BTN_INSTRUCTIONS_Y = 0.05f;
const float BTN_SETTINGS_Y = -0.20f;

// instruction card layout
const float CARD_LEFT = -0.62f;
const float CARD_TOP_Y = 0.68f;
const float CARD_W = 0.56f;
const float CARD_H = 0.52f;
const float CARD_GAP_X = 0.04f;
const float CARD_GAP_Y = 0.05f;
const float IMG_SIZE = 0.55f;

//slider pos
const float SLIDER_X = 0.2f;
const float SLIDER_WIDTH = 0.5f;

const float MUSIC_Y = 0.2f;
const float SFX_Y = -0.05f;

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
    { &g_ImgSpike, "Spike", "Slash down to bounce" },
    { &g_ImgComputer, "Computer", "Lasers/Door unlocking" },
    { &g_ImgBossRoom, "Lift Door", "Moves to the boss stage" }
};

static InstructionCard g_BuffCards[] =
{
    { &g_ImgHpRecovery, "HP Recovery", "Restores full health" },
    { &g_ImgShield, "Shield", "Blocks damage for 10s" },
    { &g_ImgDash, "Dash", "Gain dash uses" }
};

static constexpr int g_WeaponCount = static_cast<int>(sizeof(g_CombatCards) / sizeof(g_CombatCards[0]));
static constexpr int g_InteractionCount = static_cast<int>(sizeof(g_InteractionCards) / sizeof(g_InteractionCards[0]));
static constexpr int g_BuffCount = static_cast<int>(sizeof(g_BuffCards) / sizeof(g_BuffCards[0]));

// Returns true if the mouse position is inside a left-aligned menu button
// defined by its top-left x position and center y position in Normalized Device Coordinates space
static bool IsMouseOver(float mx, float my, float x, float y) noexcept
{
    return (mx >= x && mx <= x + BTN_WIDTH &&
        my >= y - BTN_HEIGHT * 0.5f &&
        my <= y + BTN_HEIGHT * 0.5f);
}

// for drawing instruction cards
static void DrawRect(
    float cx, float cy, float pw, float ph,
    float r, float g, float b, float a
)noexcept
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

// -----------------------------------------------------------------------------
// Loads textures required by the controls menu and instruction cards.
// -----------------------------------------------------------------------------
void Controls_Load()
{
    g_SettingsPanel = TextureManager::Get().LoadTexture("Assets/Images/sliderbutton.png");
    g_Background = TextureManager::Get().LoadTexture("Assets/Images/back.png");

    // instructions page textures
    g_ImgMelee = TextureManager::Get().LoadTexture("Assets/Images/controls_melee.png");
    g_ImgRanged = TextureManager::Get().LoadTexture("Assets/Images/controls_ranged.png");
    g_ImgCheckpoint = TextureManager::Get().LoadTexture("Assets/Images/controls_checkpoint.png");
    g_ImgSwitch = TextureManager::Get().LoadTexture("Assets/Images/controls_switch.png");
    g_ImgSpike = TextureManager::Get().LoadTexture("Assets/Images/controls_spike.png");
    g_ImgComputer = TextureManager::Get().LoadTexture("Assets/Images/controls_computer.png");
    g_ImgBossRoom = TextureManager::Get().LoadTexture("Assets/Images/controls_bossdoor.png");
    g_ImgHpRecovery = TextureManager::Get().LoadTexture("Assets/Images/controls_full_hp.png");
    g_ImgShield = TextureManager::Get().LoadTexture("Assets/Images/controls_shield.png");
    g_ImgDash = TextureManager::Get().LoadTexture("Assets/Images/controls_dash.png");

    std::cout << "Controls:Load" << std::endl;
}

// -----------------------------------------------------------------------------
// Initializes controls-menu state, loads button sprite-sheet settings
// from the config file, and retrieves hover/click audio assets
// -----------------------------------------------------------------------------
void Controls_Initialize()
{
    std::cout << "Controls:Initialize" << std::endl;

    rapidjson::Document doc = LoadConfig();

    if (doc.HasMember("menus") &&
        doc["menus"].IsObject() &&
        doc["menus"].HasMember("controls_menu"))
    {
        const auto& menu = doc["menus"]["controls_menu"];

        if (menu.HasMember("buttons"))
        {
            const auto& btns = menu["buttons"];

            g_ControlButtonSheet = std::make_unique<SpriteSheet>(
                btns["file"].GetString(),
                btns["rows"].GetInt(),
                btns["cols"].GetInt()
            );

            if (btns.HasMember("width"))
                g_ControlButtonWidth = btns["width"].GetFloat();

            if (btns.HasMember("height"))
                g_ControlButtonHeight = btns["height"].GetFloat();
        }
    }

    g_HoverSound = AudioManager::Get().GetAudio("hover_button");
    g_ClickSound = AudioManager::Get().GetAudio("click_button");
    g_PreviousHoveredButton = -1;
    g_CurrentTab = TAB_KEYS;
}

// -----------------------------------------------------------------------------
// Updates the standalone controls menu:
// - handles keyboard exit
// - detects button hover/click
// - switches tabs
// - updates music and SFX sliders in the settings tab
// -----------------------------------------------------------------------------
void Controls_Update()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        if (g_FromPause)
        {
            GameStateManager::Get().next = GS_MAINGAME;
            g_FromPause = false;
        }
        else
        {
            GameStateManager::Get().next = GS_MAINMENU;
        }
    }

    // mouse position
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    // Convert mouse position from screen space to normalized device coordinates
    float nx = (mx / w) * 2.0f - 1.0f;
    float ny = 1.0f - (my / h) * 2.0f;

    g_HoveredButton = CONTROLBTN_NONE;

    if (IsMouseOver(nx, ny, BTN_X, BTN_KEYS_Y))
        g_HoveredButton = CONTROLBTN_KEYS;
    else if (IsMouseOver(nx, ny, BTN_X, BTN_INSTRUCTIONS_Y))
        g_HoveredButton = CONTROLBTN_INSTRUCTIONS;
    else if (IsMouseOver(nx, ny, BTN_X, BTN_SETTINGS_Y))
        g_HoveredButton = CONTROLBTN_SETTINGS;
    else
    {
        // Convert the back button's pixel-based draw position and size into NDC
        // so hover detection matches the rendered sprite
        float backCX = -720.0f / (w * 0.5f);
        float backCY = 400.0f / (h * 0.5f);
        float backW = g_ControlButtonWidth / w * 2.0f;
        float backH = g_ControlButtonHeight / h * 2.0f;

        if (IsMouseOverRect(nx, ny, backCX, backCY, backW, backH))
            g_HoveredButton = CONTROLBTN_BACK;
    }

    //hover
    if (g_HoveredButton != CONTROLBTN_NONE && g_HoveredButton != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }
    g_PreviousHoveredButton = g_HoveredButton;

    // click
    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (g_HoveredButton == CONTROLBTN_KEYS)
        {
            g_CurrentTab = TAB_KEYS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_INSTRUCTIONS)
        {
            g_CurrentTab = TAB_INSTRUCTIONS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_SETTINGS)
        {
            g_CurrentTab = TAB_SETTINGS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_BACK)
        {
            if (g_FromPause)
            {
                GameStateManager::Get().next = GS_MAINGAME;
                g_FromPause = false;
            }
            else
            {
                GameStateManager::Get().next = GS_MAINMENU;
            }
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
    }

    if (g_CurrentTab == TAB_SETTINGS)
    {
        // start dragging
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (fabs(nx - SLIDER_X) < (SLIDER_WIDTH * 0.5f) && fabs(ny - MUSIC_Y) < 0.05f)
                g_DragMusic = true;

            if (fabs(nx - SLIDER_X) < (SLIDER_WIDTH * 0.5f) && fabs(ny - SFX_Y) < 0.05f)
                g_DragSFX = true;
        }

        // stop dragging
        if (!AEInputCheckCurr(AEVK_LBUTTON))
        {
            g_DragMusic = false;
            g_DragSFX = false;
        }

        // compute the slider's left edge so mouse x can be converted into a 0-1 value
        float sliderLeft = SLIDER_X - SLIDER_WIDTH * 0.5f;

        if (g_DragMusic)
        {
            // map cursor position along the slider bar to a normalized volume value.
            g_MusicSlider = (nx - sliderLeft) / SLIDER_WIDTH;
            g_MusicSlider = Clamp01(g_MusicSlider);

            AudioManager::Get().SetMusicVolume(g_MusicSlider);
        }

        if (g_DragSFX)
        {
            g_SFXSlider = (nx - sliderLeft) / SLIDER_WIDTH;
            g_SFXSlider = Clamp01(g_SFXSlider);

            AudioManager::Get().SetSFXVolume(g_SFXSlider);
        }
    }
}

// -----------------------------------------------------------------------------
// Updates the in-game controls overlay version of the menu.
// Similar to Controls_Update(), but closes through UIManager instead of
// changing game state when exiting
// -----------------------------------------------------------------------------
void Controls_UpdateOverlay()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        UIManager::Get().HideControls();
        return;
    }

    // mouse position
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    float nx = (mx / w) * 2.0f - 1.0f;
    float ny = 1.0f - (my / h) * 2.0f;

    g_HoveredButton = CONTROLBTN_NONE;

    if (IsMouseOver(nx, ny, BTN_X, BTN_KEYS_Y))
        g_HoveredButton = CONTROLBTN_KEYS;
    else if (IsMouseOver(nx, ny, BTN_X, BTN_INSTRUCTIONS_Y))
        g_HoveredButton = CONTROLBTN_INSTRUCTIONS;
    else if (IsMouseOver(nx, ny, BTN_X, BTN_SETTINGS_Y))
        g_HoveredButton = CONTROLBTN_SETTINGS;
    else
    {
        float backCX = -720.0f / (w * 0.5f);
        float backCY = 400.0f / (h * 0.5f);
        float backW = g_ControlButtonWidth / w * 2.0f;
        float backH = g_ControlButtonHeight / h * 2.0f;

        if (IsMouseOverRect(nx, ny, backCX, backCY, backW, backH))
            g_HoveredButton = CONTROLBTN_BACK;
    }

    if (g_HoveredButton != CONTROLBTN_NONE && g_HoveredButton != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }
    g_PreviousHoveredButton = g_HoveredButton;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (g_HoveredButton == CONTROLBTN_KEYS)
        {
            g_CurrentTab = TAB_KEYS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_INSTRUCTIONS)
        {
            g_CurrentTab = TAB_INSTRUCTIONS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_SETTINGS)
        {
            g_CurrentTab = TAB_SETTINGS;
            AudioManager::Get().PlayAudio(g_ClickSound, false);
        }
        else if (g_HoveredButton == CONTROLBTN_BACK)
        {
            UIManager::Get().HideControls();
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            return;
        }
    }

    if (g_CurrentTab == TAB_SETTINGS)
    {
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (fabs(nx - SLIDER_X) < (SLIDER_WIDTH * 0.5f) && fabs(ny - MUSIC_Y) < 0.05f)
                g_DragMusic = true;

            if (fabs(nx - SLIDER_X) < (SLIDER_WIDTH * 0.5f) && fabs(ny - SFX_Y) < 0.05f)
                g_DragSFX = true;
        }

        if (!AEInputCheckCurr(AEVK_LBUTTON))
        {
            g_DragMusic = false;
            g_DragSFX = false;
        }

        float sliderLeft = SLIDER_X - SLIDER_WIDTH * 0.5f;

        if (g_DragMusic)
        {
            g_MusicSlider = Clamp01((nx - sliderLeft) / SLIDER_WIDTH);
            AudioManager::Get().SetMusicVolume(g_MusicSlider);
        }

        if (g_DragSFX)
        {
            g_SFXSlider = Clamp01((nx - sliderLeft) / SLIDER_WIDTH);
            AudioManager::Get().SetSFXVolume(g_SFXSlider);
        }
    }
}

// -----------------------------------------------------------------------------
// Renders the keybinding reference page.
// -----------------------------------------------------------------------------
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
    Print("E - Interaction with mechanisms");
    Print("Left Click - Interaction on-screen buttons");

    y -= spacing;

    // Abilities
    FontManager::Get().Print(FontManager::Get().GetMediumFont(), "ABILITIES", startX, y, 1.2f, 1, 1, 1, 1);
    y -= spacing;

    Print("Space - Jump");
    Print("F - Weapon Switch");
    Print("1/2/3 - Inventory");
    Print("Right Click - Dash (when enabled)");
    Print("W/S + Left Click - Melee ONLY, Attack Up/Down");
    Print("Left Click - Attack");
}

// -----------------------------------------------------------------------------
// draw instructions page
// Renders the instructions tab, grouped into combat, interaction, and buff sections
// -----------------------------------------------------------------------------
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
    DrawInstructionCard(g_InteractionCards[2], rightX, y, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_InteractionCards[3], leftX, y - cardH - gapY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_InteractionCards[4], midX, y - cardH - gapY, cardW, cardH, halfW, halfH);

    // ---- Buff Section ---- //
    float buffY = y - 2.0f * (cardH + gapY) - 35.0f;
    DrawSectionTitle("BUFFS", -0.37f, buffY / halfH);

    buffY -= 70.0f;

    DrawInstructionCard(g_BuffCards[0], leftX, buffY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_BuffCards[1], midX, buffY, cardW, cardH, halfW, halfH);
    DrawInstructionCard(g_BuffCards[2], rightX, buffY, cardW, cardH, halfW, halfH);

}

// -----------------------------------------------------------------------------
// Draws the shared controls-menu contents, including tab buttons and the
// currently selected page
// -----------------------------------------------------------------------------
static void DrawControlsContents(float w, float h)
{
    float halfW = w / 2.0f;
    float halfH = h / 2.0f;

    if (g_ControlButtonSheet)
    {
        float btnX = (BTN_X * halfW) + 40.0f;

        // draws one tab button, swapping to the pressed frame and slightly enlarging it on hover
        auto DrawBtn = [&](int normalFrame, int pressedFrame, int btnId, float ndcY)
            {
                int frame = normalFrame;

                if (g_HoveredButton == btnId)
                    frame = pressedFrame;

                g_ControlButtonSheet->SetFrame(frame);

                float scale = (g_HoveredButton == btnId) ? 1.12f : 1.0f;

                MeshManager::Get().DrawSpriteSheet(
                    *g_ControlButtonSheet,
                    btnX,
                    ndcY * halfH,
                    g_ControlButtonWidth * scale,
                    g_ControlButtonHeight * scale
                );
            };

        DrawBtn(FRAME_KEYS, FRAME_KEYS_PRESSED, CONTROLBTN_KEYS, BTN_KEYS_Y);
        DrawBtn(FRAME_INSTRUCTIONS, FRAME_INSTRUCTIONS_PRESSED, CONTROLBTN_INSTRUCTIONS, BTN_INSTRUCTIONS_Y);
        DrawBtn(FRAME_SETTINGS, FRAME_SETTINGS_PRESSED, CONTROLBTN_SETTINGS, BTN_SETTINGS_Y);

        float backScale = (g_HoveredButton == CONTROLBTN_BACK) ? 1.05f : 1.0f;

        g_ControlButtonSheet->SetFrame(FRAME_BACK);

        MeshManager::Get().DrawSpriteSheet(
            *g_ControlButtonSheet,
            -720.0f,
            400.0f,
            g_ControlButtonWidth * backScale,
            g_ControlButtonHeight * backScale
        );
    }

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
        FontManager::Get().Print(FontManager::Get().GetSmallFont(),
            "Music", -0.2f, 0.2f, 1, 1, 1, 1, 1);

        FontManager::Get().Print(FontManager::Get().GetSmallFont(),
            "SFX", -0.2f, -0.05f, 1, 1, 1, 1, 1);

        DrawRect(200, 100, 300, 8, 0.6f, 0.2f, 0.8f, 1.0f);
        DrawRect(200, -20, 300, 8, 0.6f, 0.2f, 0.8f, 1.0f);

        float knobX_Music = 200 + (g_MusicSlider - 0.5f) * 300;
        float knobX_SFX = 200 + (g_SFXSlider - 0.5f) * 300;

        if (g_SettingsPanel)
        {
            MeshManager::Get().DrawTexturedSquare(g_SettingsPanel, knobX_Music, 100, 30, 30, 1.0f);
            MeshManager::Get().DrawTexturedSquare(g_SettingsPanel,  knobX_SFX, -20, 30, 30, 1.0f);
        }
    }
}

// -----------------------------------------------------------------------------
// Draws the standalone controls menu screen
// -----------------------------------------------------------------------------
void Controls_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    MeshManager::Get().DrawSquare(
        0, 0,
        w, h,
        10, 25, 60,
        180
    );

    if (g_Background)
    {
        MeshManager::Get().DrawTexturedSquare(
            g_Background,
            0, 0,
            w, h,
            0.3f
        );
    }

    DrawControlsContents(w, h);

   AESysFrameEnd();
}

// -----------------------------------------------------------------------------
// Draws the controls menu as an overlay on top of gameplay.
// -----------------------------------------------------------------------------
void Controls_DrawOverlay(float camX, float camY)
{
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float w = (float)AEGfxGetWindowWidth();
    float h = (float)AEGfxGetWindowHeight();

    // same full controls background as standalone
    MeshManager::Get().DrawSquare(
        camX, camY,
        w + 150.0f, h + 150.0f,
        10, 25, 60,
        180
    );

    if (g_Background)
    {
        MeshManager::Get().DrawTexturedSquare(
            g_Background,
            0, 0,
            w, h,
            0.3f
        );
    }

    DrawControlsContents(w, h);
}


void Controls_Free()
{
    std::cout << "Controls:Free" << std::endl;
}

// -----------------------------------------------------------------------------
// Unloads all textures used by the controls menu and resets cached pointers
// -----------------------------------------------------------------------------
void Controls_Unload()
{
    TextureManager::Get().UnloadTexture("Assets/Images/sliderbutton.png");
    TextureManager::Get().UnloadTexture("Assets/Images/back.png");

    // unload instruction page textures
    TextureManager::Get().UnloadTexture("Assets/Images/controls_melee.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_ranged.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_checkpoint.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_switch.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_spike.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_computer.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_bossdoor.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_full_hp.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_shield.png");
    TextureManager::Get().UnloadTexture("Assets/Images/controls_dash.png");

    g_BG = nullptr;
    g_SettingsPanel = nullptr;
    g_Background = nullptr;

    // instructions page
    g_ImgMelee = nullptr;
    g_ImgRanged = nullptr;
    g_ImgCheckpoint = nullptr;
    g_ImgSwitch = nullptr;
    g_ImgSpike = nullptr;
    g_ImgComputer = nullptr;
    g_ImgBossRoom = nullptr;
    g_ImgHpRecovery = nullptr;
    g_ImgShield = nullptr;
    g_ImgDash = nullptr;

    g_ControlButtonSheet.reset();

    std::cout << "Controls:Unload" << std::endl;
}
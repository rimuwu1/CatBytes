/* Start Header ************************************************************************/
/*!
\file MainMenu.cpp
\author Sim Hui Min, s.huimin, 2503506
        Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
\par s.huimin@digipen.edu
     tse.x@digipen.edu
     joash.ng@digipen.edu
\date 24/01/2026
\brief Main menu with dynamic Continue button, last saved date, and hover info panel.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Fonts.h"
#include "AudioManager.h"
#include "GameSaveManager.h"
#include "UIManager.h"
#include "Camera.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include <fstream>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include <string>
#include "TransitionManager.h"

static int frameCounter = 0;
static int g_hoveredButton = 0;
AEAudio g_MainMenuMusic{};
static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};
static int g_PreviousHoveredButton = 0;

// bacakground texture
static AEGfxTexture* g_MainMenuBG = nullptr;

// Save‑related state
static bool g_ShowContinue = false;
static std::string g_LastSavedDate;
static int g_CurrentLevel = 1;
static int g_LevelsCompleted = 0;
static int g_PlayerLives = 3;
static int g_TotalLevels = 4;   // from metadata, fallback 4

// Main game music
extern AEAudio g_GameMusic;
extern bool g_GameMusicPlaying;

// game logo
static std::unique_ptr<SpriteSheet> g_LogoAnim = nullptr;
static bool g_PlayLogoAnim = false;

static AEGfxTexture* g_Logo = nullptr;

static float g_LogoWidth = 400.0f;
static float g_LogoHeight = 200.0f;
static float g_LogoDelay = 4.5f;

// buttons
static std::unique_ptr<SpriteSheet> g_ButtonSheet = nullptr;
static float g_ButtonWidth = 300.0f;
static float g_ButtonHeight = 60.0f;

extern const char* textScreenMessage;

static rapidjson::Document LoadConfig()  
{
    rapidjson::Document doc;

    std::ifstream ifs("Assets/Data/GameConfig.json");
    rapidjson::IStreamWrapper isw(ifs);
    doc.ParseStream(isw);

    return doc;
}

// ----------------------------------------------------------------------------
// Menu layout constants (NDC: -1..1 screen range)
// ----------------------------------------------------------------------------
const float BUTTON_X = -0.9f;                // left edge of all buttons
const float BUTTON_WIDTH = 0.4f;              // wide enough to cover any text length
const float BUTTON_CLICK_HEIGHT = 0.18f;       // vertical size of clickable area
const float DETECTION_Y_OFFSET = 0.03f;        // shift hitbox up relative to text baseline

// Title position
const float TITLE_Y = 0.4f; // change here to adjust logo height -> higher value higher up position

// Y positions for buttons when Continue is shown
const float CONTINUE_Y = 0.10f;
const float NEWGAME_Y = -0.05f;
const float CONTROLS_Y = -0.20f;
const float CREDITS_Y = -0.35f;
const float EXIT_Y = -0.50f;

// Y positions when Continue is hidden (four‑button layout)
const float NEWGAME_Y_NO_CONTINUE = 0.10f;
const float CONTROLS_Y_NO_CONTINUE = -0.05f;
const float CREDITS_Y_NO_CONTINUE = -0.20f;
const float EXIT_Y_NO_CONTINUE = -0.35f;

// Info panel position (right side)
const float INFO_X = -0.50f; // Shifted left
const float INFO_START_Y = 0.15f; // Shifted down
const float INFO_LINE_SPACING = 0.08f;

// Button indices
enum MenuButton {
    BTN_NONE = 0,
    BTN_CONTINUE,
    BTN_NEWGAME,
    BTN_CONTROLS,
    BTN_CREDITS,
    BTN_EXIT
};

static void LoadAudioBankForMenu()
{
    auto tryLoadAudioFromFile = [](const char* path) -> bool
        {
            std::ifstream ifs(path);
            if (!ifs.is_open())
                return false;

            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document doc;
            doc.ParseStream(isw);

            if (doc.HasParseError() || !doc.IsObject())
                return false;

            if (!doc.HasMember("audio") || !doc["audio"].IsObject())
                return false;

            AudioManager::Get().LoadFromJson(doc["audio"]);
            return true;
        };

    // Try save first, but if save does not contain audio, fall back to config
    if (!tryLoadAudioFromFile("Assets/Data/GameSave.json"))
    {
        tryLoadAudioFromFile("Assets/Data/GameConfig.json");
    }
}

// ----------------------------------------------------------------------------
// Helper: check if mouse is over a button at given Y (baseline)
// ----------------------------------------------------------------------------
static bool IsMouseOverButton(float mouseX, float mouseY, float buttonY)
{
    float detectY = buttonY + DETECTION_Y_OFFSET;   // shift hitbox to text center
    return (mouseX >= BUTTON_X && mouseX <= BUTTON_X + BUTTON_WIDTH &&
        mouseY >= detectY - BUTTON_CLICK_HEIGHT * 0.5f &&
        mouseY <= detectY + BUTTON_CLICK_HEIGHT * 0.5f);
}

// ----------------------------------------------------------------------------

void MainMenu_Load()
{
    std::cout << "Main Menu:Load" << std::endl;

    // Preload game config once for fast saves
    GameSaveManager::PreloadConfig();

    // load background texture once at load time
    g_MainMenuBG = TextureManager::Get().LoadTexture("Assets/Images/MainMenuBackground.png");  

}

void MainMenu_Initialize()
{
    g_PlayLogoAnim = false;
    frameCounter = 0;
    LoadAudioBankForMenu();

    // Stop game music if still playing
    if (g_GameMusicPlaying)
    {
        AudioManager::Get().StopAudio(g_GameMusic);
        g_GameMusicPlaying = false;
    }

    // Start main menu music
    g_MainMenuMusic = AudioManager::Get().GetAudio("main_menu_music");
    AudioManager::Get().PlayAudio(g_MainMenuMusic, true);

    g_HoverSound = AudioManager::Get().GetAudio("hover_button");
    g_ClickSound = AudioManager::Get().GetAudio("click_button");
    g_PreviousHoveredButton = 0;

    // Clear any leftover popup state from a previous visit to this screen
    UIManager::Get().Reset();

    // ----- Check for valid save file -----
    g_ShowContinue = false;
    
    // Try cached metadata first (fast, no I/O, non-blocking)
    const auto* cached = GameSaveManager::GetCachedMetadata();
    if (cached && cached->save_date != "00-00-0000 00:00:00")
    {
        g_ShowContinue = true;
        g_LastSavedDate = cached->save_date;
        g_CurrentLevel = cached->current_level;
        g_LevelsCompleted = cached->levels_completed;
        g_PlayerLives = cached->player_lives;
    }
    else
    {
        // Fallback to reading file (for cold start, no cached data yet)
        std::ifstream saveFile("Assets/Data/GameSave.json");
        if (saveFile.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(saveFile)),
                std::istreambuf_iterator<char>());
            saveFile.close();

            rapidjson::Document doc;
            if (!doc.Parse(content.c_str()).HasParseError() && doc.IsObject())
            {
                if (doc.HasMember("metadata") && doc["metadata"].IsObject())
                {
                    const auto& meta = doc["metadata"];

                    // Save date
                    if (meta.HasMember("save_date") && meta["save_date"].IsString())
                    {
                        std::string date = meta["save_date"].GetString();
                        if (date != "00-00-0000 00:00:00")
                        {
                            g_ShowContinue = true;
                            g_LastSavedDate = date;
                        }
                    }

                    // Additional metadata for hover info
                    if (meta.HasMember("current_level") && meta["current_level"].IsInt())
                        g_CurrentLevel = meta["current_level"].GetInt();
                    if (meta.HasMember("levels_completed") && meta["levels_completed"].IsInt())
                        g_LevelsCompleted = meta["levels_completed"].GetInt();
                    if (meta.HasMember("player_lives") && meta["player_lives"].IsInt())
                        g_PlayerLives = meta["player_lives"].GetInt();
                    if (meta.HasMember("total_levels") && meta["total_levels"].IsInt())
                        g_TotalLevels = meta["total_levels"].GetInt();
                }
            }
        }
    }

    // game logo 
    rapidjson::Document doc = LoadConfig();

    if (doc.HasMember("menus") &&
        doc["menus"].HasMember("main_menu"))
    {
        const auto& menu = doc["menus"]["main_menu"];

        // -------- static --------
        if (menu.HasMember("logo"))
        {
            const auto& logo = menu["logo"];

            g_Logo = TextureManager::Get().LoadTexture(
                logo["file"].GetString()
            );

            if (logo.HasMember("width"))
                g_LogoWidth = logo["width"].GetFloat();

            if (logo.HasMember("height"))
                g_LogoHeight = logo["height"].GetFloat();
        }

        // -------- animated --------
        if (menu.HasMember("logo_anim"))
        {
            const auto& anims = menu["logo_anim"];

            g_LogoAnim = std::make_unique<SpriteSheet>(
                anims["file"].GetString(),
                anims["rows"].GetInt(),
                anims["cols"].GetInt()
            );

            if (anims.HasMember("width"))
                g_LogoWidth = anims["width"].GetFloat();

            if (anims.HasMember("height"))
                g_LogoHeight = anims["height"].GetFloat();

            if (anims.HasMember("delay"))
                g_LogoDelay = anims["delay"].GetFloat();

            const auto& clips = anims["clips"];
            for (rapidjson::SizeType i = 0; i < clips.Size(); i++)
            {
                const auto& c = clips[i];
                g_LogoAnim->AddClip(
                    c["name"].GetString(),
                    c["start"].GetInt(),
                    c["end"].GetInt(),
                    c["duration"].GetFloat(),
                    c["loop"].GetBool()
                );
            }
        }
        // buttons 
        if (menu.HasMember("buttons"))
        {
            const auto& btns = menu["buttons"];
            g_ButtonSheet = std::make_unique<SpriteSheet>(
                btns["file"].GetString(),
                btns["rows"].GetInt(),
                btns["cols"].GetInt()
            );
            if (btns.HasMember("width"))  g_ButtonWidth  = btns["width"].GetFloat();
            if (btns.HasMember("height")) g_ButtonHeight = btns["height"].GetFloat();
        }
    }

    std::cout << "Main Menu:Initialize" << std::endl;
}

void MainMenu_Update()
{
    //Let UIManager process any active popup first
    AEGfxSetCamPosition(0.0f, 0.0f); //clear camera settings
    if (UIManager::Get().Update(0,0)) {
        return;
    }
    frameCounter++;

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    float windowWidth = (float)AEGfxGetWindowWidth();
    float windowHeight = (float)AEGfxGetWindowHeight();
    float normalizedX = ((float)mouseX / windowWidth) * 2.0f - 1.0f;
    float normalizedY = 1.0f - ((float)mouseY / windowHeight) * 2.0f;

    int newHover = BTN_NONE;

    if (g_ShowContinue)
    {
        if (IsMouseOverButton(normalizedX, normalizedY, CONTINUE_Y))
            newHover = BTN_CONTINUE;
        else if (IsMouseOverButton(normalizedX, normalizedY, NEWGAME_Y))
            newHover = BTN_NEWGAME;
        else if (IsMouseOverButton(normalizedX, normalizedY, CONTROLS_Y))
            newHover = BTN_CONTROLS;
        else if (IsMouseOverButton(normalizedX, normalizedY, CREDITS_Y))
            newHover = BTN_CREDITS;
        else if (IsMouseOverButton(normalizedX, normalizedY, EXIT_Y))
            newHover = BTN_EXIT;
    }
    else
    {
        if (IsMouseOverButton(normalizedX, normalizedY, NEWGAME_Y_NO_CONTINUE))
            newHover = BTN_NEWGAME;
        else if (IsMouseOverButton(normalizedX, normalizedY, CONTROLS_Y_NO_CONTINUE))
            newHover = BTN_CONTROLS;
        else if (IsMouseOverButton(normalizedX, normalizedY, CREDITS_Y_NO_CONTINUE))
            newHover = BTN_CREDITS;
        else if (IsMouseOverButton(normalizedX, normalizedY, EXIT_Y_NO_CONTINUE))
            newHover = BTN_EXIT;
    }

    // Handle clicks (disabled during transition)
    if (!TransitionManager::Get().IsActive() && newHover != BTN_NONE && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        switch (newHover)
        {
        case BTN_CONTINUE:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            TransitionManager::Get().Start(GS_MAINGAME);
            break;
        case BTN_NEWGAME:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            if (g_ShowContinue) {
                UIManager::Get().ShowConfirmation(
                    "Start New Game?",
                    "All saved progress will be cleared!",
                    []() {
                        g_newGame = true;
                        GameSaveManager::ResetSave();
                        textScreenMessage = "Cutscene";
                        TransitionManager::Get().Start(GS_WINLOSE);
                    },
                    []() { /* cancel – do nothing */ },
                    globalCam.x, globalCam.y
                );
            }
            else {
                GameSaveManager::ResetSave();
                /*TransitionManager::Get().Start(GS_MAINGAME);*/
                textScreenMessage = "Cutscene";
                TransitionManager::Get().Start(GS_WINLOSE);
            }
            break;
        case BTN_CONTROLS:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            TransitionManager::Get().Start(GS_CONTROLS);
            break;
        case BTN_CREDITS:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            TransitionManager::Get().Start(GS_CREDITS);
            break;
        case BTN_EXIT:
                UIManager::Get().ShowConfirmation(
                    "Quit Game?",
                    "Are You Sure You Want To Quit?",
                    []() {
                        TransitionManager::Get().Start(GS_QUIT);
                    },
                    []() { /* cancel – do nothing */ },
                    globalCam.x, globalCam.y
                );
            break;
        }
    }

    // Hover sound
    if (newHover != BTN_NONE && newHover != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }

    g_hoveredButton = newHover;
    g_PreviousHoveredButton = g_hoveredButton;

    std::cout << "Main Menu:Update" << std::endl;

    // logo delay then trigger spritesheet animation 
    float time = frameCounter / 60.0f;

    if (!g_PlayLogoAnim && time > g_LogoDelay)
    {
        g_PlayLogoAnim = true;

        if (g_LogoAnim)
            g_LogoAnim->Play("intro");
    }

    TransitionManager::Get().Update(static_cast<float>(AEFrameRateControllerGetFrameTime()));
}

void MainMenu_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

    // menu background
    if (g_MainMenuBG)
    {
        float w = static_cast<float>(AEGfxGetWindowWidth());
        float h = static_cast<float>(AEGfxGetWindowHeight());
        MeshManager::Get().DrawTexturedSquare(g_MainMenuBG, 0.0f, 0.0f, w, h, 1.0f);
    }

    // draw logo (static for delay, then animated)
    float logoCenterX = (BUTTON_X * (AEGfxGetWindowWidth() / 2.0f)) + (g_LogoWidth / 2.0f);
    float logoCenterY = TITLE_Y * (AEGfxGetWindowHeight() / 2.0f);

    if (g_PlayLogoAnim && g_LogoAnim)
    {
        g_LogoAnim->Update(1.0f / 60.0f);
        MeshManager::Get().DrawSpriteSheet(*g_LogoAnim, logoCenterX, logoCenterY, g_LogoWidth, g_LogoHeight);
    }
    else if (g_Logo)
    {
        MeshManager::Get().DrawTexturedSquare(g_Logo, logoCenterX, logoCenterY, g_LogoWidth, g_LogoHeight, 1.0f);
    }

    //if (g_FontLarge != -1 && g_FontMedium != -1)
    //{

    //    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    //    if (g_ShowContinue)
    //    {
    //        // Continue button with last saved date
    //        float r = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
    //        float g = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
    //        float b = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
    //        std::string continueText = "CONTINUE"; // (Last saved: " + g_LastSavedDate + ")
    //        AEGfxPrint(g_FontMedium, continueText.c_str(), BUTTON_X, CONTINUE_Y, 1.0f, r, g, b, 1.0f);

    //        // New Game
    //        r = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "NEW GAME", BUTTON_X, NEWGAME_Y, 1.0f, r, g, b, 1.0f);

    //        // Controls
    //        r = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, CONTROLS_Y, 1.0f, r, g, b, 1.0f);

    //        // Credits
    //        r = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, CREDITS_Y, 1.0f, r, g, b, 1.0f);

    //        // Exit
    //        r = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, EXIT_Y, 1.0f, r, g, b, 1.0f);

    //        // ---------- Hover info for Continue button ----------
    //        if (g_hoveredButton == BTN_CONTINUE)
    //        {
    //            s8 font = (g_FontSmall != -1) ? g_FontSmall : g_FontMedium;
    //            float scale = (g_FontSmall != -1) ? 1.0f : 0.6f;
    //            float y = INFO_START_Y;

    //            // Level progress
    //            std::string line1 = "Level " + std::to_string(g_CurrentLevel) + " / " + std::to_string(g_TotalLevels);
    //            AEGfxPrint(font, line1.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
    //            y -= INFO_LINE_SPACING;

    //            // Levels completed
    //            std::string line2 = "Completed: " + std::to_string(g_LevelsCompleted);
    //            AEGfxPrint(font, line2.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
    //            y -= INFO_LINE_SPACING;

    //            // Player lives
    //            std::string line3 = "Lives: " + std::to_string(g_PlayerLives) + "/3";
    //            AEGfxPrint(font, line3.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
    //            y -= INFO_LINE_SPACING;

    //            // Last saved date (again)
    //            std::string line4 = "Saved: " + g_LastSavedDate;
    //            AEGfxPrint(font, line4.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
    //        }
    //    }
    //    else
    //    {
    //        // No Continue – New Game is first
    //        float r = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        float g = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        float b = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "NEW GAME", BUTTON_X, NEWGAME_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

    //        // Controls
    //        r = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, CONTROLS_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

    //        // Credits
    //        r = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, CREDITS_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

    //        // Exit
    //        r = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        g = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        b = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
    //        AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, EXIT_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);
    //    }
    //}
    

    // button with spritesheet
    if (g_ButtonSheet)
    {
        float halfH = AEGfxGetWindowHeight() / 2.0f;
        float btnX = (BUTTON_X * (AEGfxGetWindowWidth() / 2.0f)) + (g_ButtonWidth / 2.0f);

        auto DrawButton = [&](int normalFrame, int hoverFrame, int btnId, float ndcY)
            {
                int frame = (g_hoveredButton == btnId) ? hoverFrame : normalFrame;
                g_ButtonSheet->SetFrame(frame);
                MeshManager::Get().DrawSpriteSheet(*g_ButtonSheet, btnX, ndcY * halfH, g_ButtonWidth, g_ButtonHeight);
            };

        if (g_ShowContinue)
        {
            DrawButton(0, 1, BTN_CONTINUE, CONTINUE_Y);
            DrawButton(2, 3, BTN_NEWGAME,  NEWGAME_Y);
            DrawButton(4, 5, BTN_CONTROLS, CONTROLS_Y);
            DrawButton(6, 7, BTN_CREDITS,  CREDITS_Y);
            DrawButton(8, 9, BTN_EXIT,     EXIT_Y);

            // hover info panel (keep this as text)
            if (g_hoveredButton == BTN_CONTINUE)
            {
                // Convert NDC coordinates to pixel coordinates for DrawSquare
                // Text spans from 0.15f down to -0.17f NDC (4 lines × 0.08f)
                // Box needs to cover: ~0.17f to ~-0.19f (with padding)
                
                const float BOX_CENTER_Y_NDC = 0.05f; // Slightly up
                const float BOX_HEIGHT_NDC = 0.36f; // Covers 4 lines + padding
                const float BOX_WIDTH_PX = 360.0f; // Pixel width
                const float BOX_HEIGHT_PX = BOX_HEIGHT_NDC * halfH; // Convert NDC to pixels
                const float BOX_X_PX = 0.0f; // Center with text
                const float BOX_Y_PX = BOX_CENTER_Y_NDC * halfH; // Center box on text
                
                // Draw translucent dark background box
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                MeshManager::Get().DrawSquare(BOX_X_PX, BOX_Y_PX, BOX_WIDTH_PX, BOX_HEIGHT_PX, 0, 0, 0, 0.8f);
                
                // Reset render mode for text
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                
                s8 font = FontManager::Get().GetSmallFont();
                float scale = 1.0f;
                float y = INFO_START_Y;
                float x = 0.0f; // Center
                
                std::string line1 = "Level " + std::to_string(g_CurrentLevel) + " / " + std::to_string(g_TotalLevels);
                FontManager::Get().PrintThemeAligned(font, line1.c_str(), x, y, scale, TextAlignment::Center, FontTheme::Gold); y -= INFO_LINE_SPACING;
                std::string line2 = "Completed: " + std::to_string(g_LevelsCompleted);
                FontManager::Get().PrintThemeAligned(font, line2.c_str(), x, y, scale, TextAlignment::Center, FontTheme::White); y -= INFO_LINE_SPACING;
                std::string line3 = "Lives: " + std::to_string(g_PlayerLives) + " / 5";
                FontManager::Get().PrintThemeAligned(font, line3.c_str(), x, y, scale, TextAlignment::Center, FontTheme::White); y -= INFO_LINE_SPACING;
                std::string line4 = "Saved: " + g_LastSavedDate;
                FontManager::Get().PrintThemeAligned(font, line4.c_str(), x, y, scale, TextAlignment::Center, FontTheme::Dim);
            }
        }
        else
        {
            DrawButton(2, 3, BTN_NEWGAME,  NEWGAME_Y_NO_CONTINUE);
            DrawButton(4, 5, BTN_CONTROLS, CONTROLS_Y_NO_CONTINUE);
            DrawButton(6, 7, BTN_CREDITS,  CREDITS_Y_NO_CONTINUE);
            DrawButton(8, 9, BTN_EXIT,     EXIT_Y_NO_CONTINUE);
        }
    }

    //pop up draw
    UIManager::Get().Draw(0, 0);
    TransitionManager::Get().Draw();
    AESysFrameEnd();
    std::cout << "Main Menu:Draw" << std::endl;
}

void MainMenu_Free()
{
    std::cout << "Main Menu:Free" << std::endl;
}

void MainMenu_Unload()
{
    TextureManager::Get().UnloadTexture("Assets/Images/MainMenuBackground.png");
    g_MainMenuBG = nullptr;

    if (g_Logo)
    {
        TextureManager::Get().UnloadTexture("Assets/Images/titleicon.png"); 
        g_Logo = nullptr;
    }
    g_LogoAnim.reset();  

    g_ButtonSheet.reset();

    std::cout << "Main Menu:Unload" << std::endl;
}
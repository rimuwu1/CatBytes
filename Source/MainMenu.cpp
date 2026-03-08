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
#include "Audio.h"
#include "GameSaveManager.h"
#include "UIManager.h"
#include "Camera.h"

#include <fstream>
#include <sstream>
#include "rapidjson/document.h"
#include <string>

static int frameCounter = 0;
static int g_hoveredButton = 0;
AEAudio g_MainMenuMusic{};
static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};
static int g_PreviousHoveredButton = 0;

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

// ----------------------------------------------------------------------------
// Menu layout constants (NDC: -1..1 screen range)
// ----------------------------------------------------------------------------
const float BUTTON_X = -0.9f;                // left edge of all buttons
const float BUTTON_WIDTH = 1.5f;              // wide enough to cover any text length
const float BUTTON_CLICK_HEIGHT = 0.08f;       // vertical size of clickable area
const float DETECTION_Y_OFFSET = 0.03f;        // shift hitbox up relative to text baseline

// Title position
const float TITLE_Y = 0.3f;

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
const float INFO_X = -0.40f; //0.4 if show last saved
const float INFO_START_Y = 0.25f;
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
}

void MainMenu_Initialize()
{
    frameCounter = 0;

    // Stop game music if still playing
    if (g_GameMusicPlaying)
    {
        AudioManager::Get().StopAudio(g_GameMusic);
        g_GameMusicPlaying = false;
    }

    // Start main menu music
    g_MainMenuMusic = AudioManager::Get().LoadAudio(Audio::MAIN_MENU_MUSIC, true);
    AudioManager::Get().PlayAudio(g_MainMenuMusic, true);

    g_HoverSound = AudioManager::Get().LoadAudio(Audio::HOVER_BUTTON, false);
    g_ClickSound = AudioManager::Get().LoadAudio(Audio::CLICK_BUTTON, false);
    g_PreviousHoveredButton = 0;

    // Clear any leftover popup state from a previous visit to this screen
    UIManager::Get().Reset();

    // ----- Check for valid save file -----
    g_ShowContinue = false;
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

    // Handle clicks
    if (newHover != BTN_NONE && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        switch (newHover)
        {
        case BTN_CONTINUE:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GS_MAINGAME;
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
                        GameStateManager::Get().next = GS_MAINGAME;
                    },
                    []() { /* cancel – do nothing */ }
                );
            }
            else {
                GameSaveManager::ResetSave();
                GameStateManager::Get().next = GS_MAINGAME;
            }
            break;
        case BTN_CONTROLS:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GS_CONTROLS;
            break;
        case BTN_CREDITS:
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GS_CREDITS;
            break;
        case BTN_EXIT:
                UIManager::Get().ShowConfirmation(
                    "Quit Game?",
                    "Are You Sure You Want To Quit?",
                    []() {
                        GameStateManager::Get().next = GS_QUIT;
                    },
                    []() { /* cancel – do nothing */ }
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
}

void MainMenu_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

    if (g_FontLarge != -1 && g_FontMedium != -1)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Title
        AEGfxPrint(g_FontLarge, "POGBA", BUTTON_X, TITLE_Y, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        if (g_ShowContinue)
        {
            // Continue button with last saved date
            float r = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
            float g = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
            float b = (g_hoveredButton == BTN_CONTINUE) ? 1.0f : 0.6f;
            std::string continueText = "CONTINUE"; // (Last saved: " + g_LastSavedDate + ")
            AEGfxPrint(g_FontMedium, continueText.c_str(), BUTTON_X, CONTINUE_Y, 1.0f, r, g, b, 1.0f);

            // New Game
            r = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "NEW GAME", BUTTON_X, NEWGAME_Y, 1.0f, r, g, b, 1.0f);

            // Controls
            r = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, CONTROLS_Y, 1.0f, r, g, b, 1.0f);

            // Credits
            r = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, CREDITS_Y, 1.0f, r, g, b, 1.0f);

            // Exit
            r = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, EXIT_Y, 1.0f, r, g, b, 1.0f);

            // ---------- Hover info for Continue button ----------
            if (g_hoveredButton == BTN_CONTINUE)
            {
                s8 font = (g_FontSmall != -1) ? g_FontSmall : g_FontMedium;
                float scale = (g_FontSmall != -1) ? 1.0f : 0.6f;
                float y = INFO_START_Y;

                // Level progress
                std::string line1 = "Level " + std::to_string(g_CurrentLevel) + " / " + std::to_string(g_TotalLevels);
                AEGfxPrint(font, line1.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
                y -= INFO_LINE_SPACING;

                // Levels completed
                std::string line2 = "Completed: " + std::to_string(g_LevelsCompleted);
                AEGfxPrint(font, line2.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
                y -= INFO_LINE_SPACING;

                // Player lives
                std::string line3 = "Lives: " + std::to_string(g_PlayerLives) + "/3";
                AEGfxPrint(font, line3.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
                y -= INFO_LINE_SPACING;

                // Last saved date (again)
                std::string line4 = "Saved: " + g_LastSavedDate;
                AEGfxPrint(font, line4.c_str(), INFO_X, y, scale, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        else
        {
            // No Continue – New Game is first
            float r = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            float g = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            float b = (g_hoveredButton == BTN_NEWGAME) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "NEW GAME", BUTTON_X, NEWGAME_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

            // Controls
            r = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_CONTROLS) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, CONTROLS_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

            // Credits
            r = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_CREDITS) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, CREDITS_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);

            // Exit
            r = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            g = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            b = (g_hoveredButton == BTN_EXIT) ? 1.0f : 0.6f;
            AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, EXIT_Y_NO_CONTINUE, 1.0f, r, g, b, 1.0f);
        }
    }
    //pop up draw
    UIManager::Get().Draw(0, 0);
    AESysFrameEnd();
    std::cout << "Main Menu:Draw" << std::endl;
}

void MainMenu_Free()
{
    std::cout << "Main Menu:Free" << std::endl;
}

void MainMenu_Unload()
{
    std::cout << "Main Menu:Unload" << std::endl;
}
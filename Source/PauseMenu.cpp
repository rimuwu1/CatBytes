/* Start Header ************************************************************************/
/*!
\file PauseMenu.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief implementation for the pause menu state which handles restart, pause and interactions with game saves.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "PauseMenu.h"
#include "GameStateManager.h"
#include "Fonts.h"
#include "FileManager.h"
#include "AudioManager.h"
#include "Audio.h"

bool g_newGame = false;
static int frameCounter = 0;
static int g_hoveredButton = 0;
static int g_PreviousHoveredButton = 0;
static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};

// Pause menu buttons
const float BUTTON_X = -0.4f;           // center-aligned
const float BUTTON_START_Y = 0.1f;
const float BUTTON_SPACING = 0.15f;     // space between buttons
const float BUTTON_WIDTH = 0.4f;        // approximate width for click detection
const float BUTTON_HEIGHT = 0.1f;

void Pause_Load()
{
    std::cout << "Pause Menu:Load" << std::endl;
}

void Pause_Initialize()
{
    //initialize hover audio
    g_HoverSound = AudioManager::Get().LoadAudio(Audio::HOVER_BUTTON, false);
    g_ClickSound = AudioManager::Get().LoadAudio(Audio::CLICK_BUTTON, false);
    g_PreviousHoveredButton = 0;

    std::cout << "Pause Menu:Initialize" << std::endl;
}

void Pause_Update()
{
    // get mouse position
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    float windowWidth = (float)AEGfxGetWindowWidth();
    float windowHeight = (float)AEGfxGetWindowHeight();
    float normalizedX = ((float)mouseX / windowWidth) * 2.0f - 1.0f;
    float normalizedY = 1.0f - ((float)mouseY / windowHeight) * 2.0f;

    // hover effect
    g_hoveredButton = 0;

    // RESUME button
    if (normalizedX >= BUTTON_X && normalizedX <= BUTTON_X + BUTTON_WIDTH &&
        normalizedY >= BUTTON_START_Y && normalizedY <= BUTTON_START_Y + BUTTON_HEIGHT)
    {
        g_hoveredButton = 1;
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GameStateManager::Get().previous; // Resume game
        }
    }
    // RESTART button
    else if (normalizedX >= BUTTON_X && normalizedX <= BUTTON_X + BUTTON_WIDTH &&
        normalizedY >= BUTTON_START_Y - BUTTON_SPACING &&
        normalizedY <= BUTTON_START_Y - BUTTON_SPACING + BUTTON_HEIGHT)
    {
        g_hoveredButton = 2;
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            g_newGame = true; //flag to restart
            GameSave::ResetSave(); //clear all saves and start from beginning
            GameStateManager::Get().next = GameStateManager::Get().previous; // Go back to level
        }
    }
    // MAIN MENU button
    else if (normalizedX >= BUTTON_X && normalizedX <= BUTTON_X + BUTTON_WIDTH &&
        normalizedY >= BUTTON_START_Y - BUTTON_SPACING * 2 &&
        normalizedY <= BUTTON_START_Y - BUTTON_SPACING * 2 + BUTTON_HEIGHT)
    {
        g_hoveredButton = 3;
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            g_newGame = true; //flag to restart
            GameStateManager::Get().next = GS_MAINMENU;
        }
    }
    // EXIT button
    else if (normalizedX >= BUTTON_X && normalizedX <= BUTTON_X + BUTTON_WIDTH &&
        normalizedY >= BUTTON_START_Y - BUTTON_SPACING * 3 &&
        normalizedY <= BUTTON_START_Y - BUTTON_SPACING * 3 + BUTTON_HEIGHT)
    {
        g_hoveredButton = 4;
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GS_QUIT;
        }
    }

    // Handle ESC key to resume
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        GameStateManager::Get().next = GameStateManager::Get().previous;
    }

    //play hover sound when entering a new button
    if (g_hoveredButton != 0 && g_hoveredButton != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }

    g_PreviousHoveredButton = g_hoveredButton;

    std::cout << "Pause Menu:Update" << std::endl;
}

void Pause_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

    if (g_FontLarge != -1 && g_FontMedium != -1)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Pause title
        AEGfxPrint(g_FontLarge, "PAUSED", BUTTON_X, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        // RESUME
        float resumeColour = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "RESUME GAME", BUTTON_X, BUTTON_START_Y, 1.0f, resumeColour, resumeColour, resumeColour, 1.0f);

        // RESTART
        float restartColour = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "RESTART LEVEL", BUTTON_X, BUTTON_START_Y - BUTTON_SPACING, 1.0f, restartColour, restartColour, restartColour, 1.0f);

        // MAIN MENU
        float mainMenuColour = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "EXIT TO MAIN MENU", BUTTON_X, BUTTON_START_Y - BUTTON_SPACING * 2, 1.0f, mainMenuColour, mainMenuColour, mainMenuColour, 1.0f);

        // EXIT
        float exitColour = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "EXIT GAME", BUTTON_X, BUTTON_START_Y - BUTTON_SPACING * 3, 1.0f, exitColour, exitColour, exitColour, 1.0f);
    }

    AESysFrameEnd();
    std::cout << "Pause Menu:Draw" << std::endl;
}

void Pause_Free()
{
    std::cout << "Pause Menu:Free" << std::endl;
}

void Pause_Unload()
{
    std::cout << "Pause Menu:Unload" << std::endl;
}

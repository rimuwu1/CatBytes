/* Start Header ************************************************************************/
/*!
\file MainMenu.cpp
\author Sim Hui Min, s.huimin, 2503506
        Tse Xuan Qi Tristin, tse.x, 2503757
\par s.huimin@digipen.edu
     tse.x@digipen.edu
\date 24/01/2026
\brief 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Fonts.h"
#include "AudioManager.h"
#include "Audio.h"

static int frameCounter = 0;
static int g_hoveredButton = 0;
AEAudio g_MainMenuMusic{};
static AEAudio g_HoverSound{};
static AEAudio g_ClickSound{};
static int g_PreviousHoveredButton = 0;

//main game music
extern AEAudio g_GameMusic;
extern bool g_GameMusicPlaying;

// main menu buttons
const float BUTTON_X = -0.9f;           // left-aligned with title
const float BUTTON_START_Y = 0.3f;      
const float BUTTON_SPACING = 0.15f;     // space between buttons
const float BUTTON_WIDTH = 0.4f;        // approximate width for click detection
const float BUTTON_HEIGHT = 0.1f;

void MainMenu_Load()
{
	std::cout << "Main Menu:Load" << std::endl;
}

void MainMenu_Initialize()
{
    frameCounter = 0;

    // stop game music if still playing
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



    std::cout << "Main Menu:Initialize" << std::endl;
}

void MainMenu_Update()
{ 
    frameCounter++;

    // get mouse position
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    float windowWidth = (float)AEGfxGetWindowWidth();
    float windowHeight = (float)AEGfxGetWindowHeight();
    float normalizedX = ((float)mouseX / windowWidth) * 2.0f - 1.0f;
    float normalizedY = 1.0f - ((float)mouseY / windowHeight) * 2.0f;
    normalizedX *= 0.5f;
    normalizedY *= 0.5f;

    // hover effect
    g_hoveredButton = 0;

    // PLAY button
    if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= 0.03f && normalizedY <= 0.10f) // manually adjust the width n range here -> diff is the range (currently 0.07 diff)
    {
        g_hoveredButton = 1;  
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            GameStateManager::Get().next = GS_MAINGAME;
        }
    }
    // CONTROLS button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.06f && normalizedY <= 0.01f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 2;  
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            // TODO: Controls state
            GameStateManager::Get().next = GS_CONTROLS;
        }
    }
    // CREDITS button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.14f && normalizedY <= -0.07f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 3; 
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            AudioManager::Get().PlayAudio(g_ClickSound, false);
            // TODO: Credits state
            GameStateManager::Get().next = GS_CREDITS;
        }
    }
    // EXIT button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.20f && normalizedY <= -0.13f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 4; 
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            GameStateManager::Get().next = GS_QUIT;
        }
    }

    //play hover sound when hovering over a new button
    if (g_hoveredButton != 0 && g_hoveredButton != g_PreviousHoveredButton)
    {
        AudioManager::Get().PlayAudio(g_HoverSound, false);
    }

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

        // awesome title #noproblem
        AEGfxPrint(g_FontLarge, "POGBA", BUTTON_X, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        // PLAY - white if hovered, grey otherwise
        float playR = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        float playG = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        float playB = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "PLAY", BUTTON_X, 0.1f, 1.0f, playR, playG, playB, 1.0f);

        // CONTROLS
        float controlR = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        float controlG = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        float controlB = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, -0.05f, 1.0f, controlR, controlG, controlB, 1.0f);

        // CREDITS
        float creditR = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        float creditG = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        float creditB = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, -0.2f, 1.0f, creditR, creditG, creditB, 1.0f);

        // EXIT
        float exitR = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        float exitG = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        float exitB = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, -0.35f, 1.0f, exitR, exitG, exitB, 1.0f);
    }

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
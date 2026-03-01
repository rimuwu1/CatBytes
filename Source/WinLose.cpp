/* Start Header *****/
/*!
\file       WinLose.cpp
\author     Sim Hui Min, Huimin, s.huimin, 2503506
\par        s.huimin@digipen.edu
\date       February 01 2026
\brief      Implements the Game Over screen state functions.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#include "pch.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "WinLose.h"
#include "Fonts.h"
#include "AudioManager.h"
#include "Audio.h"

const char* textScreenMessage = "You Lose";

static AEAudio s_WinSound{};
static AEAudio s_LoseSound{};
static bool s_SoundPlayed = false;

void WinLose_Load()
{
    s_WinSound = AudioManager::Get().LoadAudio(Audio::WIN_EFFECT, false);
    s_LoseSound = AudioManager::Get().LoadAudio(Audio::LOSE_EFFECT, false);
}

void WinLose_Initialize()
{
    s_SoundPlayed = false;
}

void WinLose_Update()
{
    //play win/lose sound once when entering screen
    if (!s_SoundPlayed)
    {
        if (strcmp(textScreenMessage, "You Win") == 0)
        {
            AudioManager::Get().PlayAudio(s_WinSound, false);
        }
        else
        {
            AudioManager::Get().PlayAudio(s_LoseSound, false);
        }

        s_SoundPlayed = true;
    }


	// press any key to go back to main menu
	if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_RETURN))
	{
		GameStateManager::Get().next = GS_MAINMENU;
	}
}

void WinLose_Draw()
{
    AESysFrameStart();

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0, 0, 0, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    // main text 
    AEGfxPrint(g_FontLarge,
        textScreenMessage,
        -0.2f, // center
        0.0f,
        1.0f,
        1.0f, 1.0f, 1.0f, 1.0f);

    // sub text
    AEGfxPrint(g_FontMedium,
        "Press Space to return",
        -0.16f,
        -0.3f,
        0.5f,
        0.5f, 0.5f, 0.5f, 0.5f);

    AESysFrameEnd();
}

void WinLose_Free()
{
}

void WinLose_Unload()
{
}

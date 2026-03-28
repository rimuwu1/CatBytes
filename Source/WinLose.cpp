/* Start Header *****/
/*!
\file       WinLose.cpp
\author     Sim Hui Min, Huimin, s.huimin, 2503506
            Tse Xuan Qi Tristin, tse.x, 2503757
\par        s.huimin@digipen.edu
            tse.x@digipen.edu
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

const char* textScreenMessage = "You Lose";

static AEAudio s_WinSound{};
static AEAudio s_LoseSound{};
static bool s_SoundPlayed = false;

void WinLose_Load()
{
    s_WinSound = AudioManager::Get().GetAudio("win_effect");
    s_LoseSound = AudioManager::Get().GetAudio("lose_effect");
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
    FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(),
        textScreenMessage,
        0.0f,
        0.0f,
        1.0f,
        1.0f, 1.0f, 1.0f, 1.0f);

    // sub text
    FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(),
        "Press Space to return",
        0.0f,
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

/* Start Header ************************************************************************/
/*!
\file GameStateManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief  This file implements the GameStateManager singleton, responsible for managing
        the flow of different game states and their associated load/init/update/draw/
        free/unload function pointers.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "MainGame.h"
#include "MainMenu.h"
#include "SplashScreen.h"
#include "WinLose.h"
#include "PauseMenu.h"
#include "Controls.h"
#include "Credits.h"

// ----------------------------------------------------------------------------
// Initializes the Game State Manager with a starting state
// Parameters:
//   startingState: The initial game state enum to begin with
// ----------------------------------------------------------------------------
void GameStateManager::Initialize(int startingState)
{
    current = previous = next = startingState;
    std::cout << "GSM:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates the Game State Manager and sets function pointers for the current state
// This function should be called every frame to manage state transitions
// ----------------------------------------------------------------------------
void GameStateManager::Update()
{
    std::cout << "GSM:Update" << std::endl;

    // Determine which set of state functions to use based on the current state
    switch (current)
    {
    case GS_SPLASHSCREEN:
        fpLoad = SplashScreen_Load;
        fpInitialize = SplashScreen_Initialize;
        fpUpdate = SplashScreen_Update;
        fpDraw = SplashScreen_Draw;
        fpFree = SplashScreen_Free;
        fpUnload = SplashScreen_Unload;
        break;
    case GS_MAINMENU:
        fpLoad = MainMenu_Load;
        fpInitialize = MainMenu_Initialize;
        fpUpdate = MainMenu_Update;
        fpDraw = MainMenu_Draw;
        fpFree = MainMenu_Free;
        fpUnload = MainMenu_Unload;
        break;
    case GS_MAINGAME:
        fpLoad = MainGame_Load;
        fpInitialize = MainGame_Initialize;
        fpUpdate = MainGame_Update;
        fpDraw = MainGame_Draw;
        fpFree = MainGame_Free;
        fpUnload = MainGame_Unload;
        break;
    case GS_WINLOSE:
        fpLoad = WinLose_Load;
        fpInitialize = WinLose_Initialize;
        fpUpdate = WinLose_Update;
        fpDraw = WinLose_Draw;
        fpFree = WinLose_Free;
        fpUnload = WinLose_Unload;
        break;
    case GS_RESTART:
        break;
    case GS_CONTROLS:
        fpLoad = Controls_Load;
        fpInitialize = Controls_Initialize;
        fpUpdate = Controls_Update;
        fpDraw = Controls_Draw;
        fpFree = Controls_Free;
        fpUnload = Controls_Unload;
        break;
    case GS_CREDITS:
        fpLoad = Credits_Load;
        fpInitialize = Credits_Initialize;
        fpUpdate = Credits_Update;
        fpDraw = Credits_Draw;
        fpFree = Credits_Free;
        fpUnload = Credits_Unload;
        break;
    case GS_QUIT:
        break;
    case GS_PAUSE:
        fpLoad = Pause_Load;
        fpInitialize = Pause_Initialize;
        fpUpdate = Pause_Update;
        fpDraw = Pause_Draw;
        fpFree = Pause_Free;
        fpUnload = Pause_Unload;
        break;
    default:
        break;
    }
}
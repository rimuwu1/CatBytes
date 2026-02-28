/* Start Header ************************************************************************/
/*!
\file GameStateManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief  This file implements the functions for the Game State Manager.
		The GSM is responsible for managing the flow of different game states and
		associated functions, such as loading, updating, and drawing for each state.

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
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Boss.h"
#include "MainMenu.h"
#include "SplashScreen.h"
#include "WinLose.h"
#include "PauseMenu.h"

int current = 0, previous = 0, next = 0;

FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr, fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

// ----------------------------------------------------------------------------
// Initializes the Game State Manager with a starting state
// Parameters:
//   startingState: The initial game state enum to begin with
// ----------------------------------------------------------------------------
void GSM_Initialize(int startingState)
{
	current = previous = next = startingState; // Set all state trackers to the starting state
	std::cout << "GSM:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates the Game State Manager and sets function pointers for the current state
// This function should be called every frame to manage state transitions
// ----------------------------------------------------------------------------
void GSM_Update()
{
	std::cout << "GSM:Update" << std::endl;

	// Determine which set of state functions to use based on the current state
	switch (current)
	{
	case GS_SPLASHSCREEN:  // Splash Screen state
		fpLoad = SplashScreen_Load;
		fpInitialize = SplashScreen_Initialize;
		fpUpdate = SplashScreen_Update;
		fpDraw = SplashScreen_Draw;
		fpFree = SplashScreen_Free;
		fpUnload = SplashScreen_Unload;
		break;
	case GS_MAINMENU:  // Main Menu state
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Initialize;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
		break;
	case GS_MAINGAME: // Level 1 game state
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
	case GS_RESTART: // State indicating the game should restart
		break;
	case GS_QUIT: // State indicating the game should quit/exit
		break;
	case GS_PAUSE:
		fpLoad = Pause_Load;
		fpInitialize = Pause_Initialize;
		fpUpdate = Pause_Update;
		fpDraw = Pause_Draw;
		fpFree = Pause_Free;
		fpUnload = Pause_Unload;
		break;
	default: // Handle any undefined state IDs
		break;
	}

}
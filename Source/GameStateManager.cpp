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
#include "Level1.h"
#include "Level2.h"
#include "MainMenu.h"

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
	case GS_MAINMENU:  // Main Menu state
		fpLoad = MainMenu_Load;
		fpInitialize = MainMenu_Initialize;
		fpUpdate = MainMenu_Update;
		fpDraw = MainMenu_Draw;
		fpFree = MainMenu_Free;
		fpUnload = MainMenu_Unload;
		break;
	case GS_LEVEL1: // Level 1 game state
		fpLoad = Level1_Load;
		fpInitialize = Level1_Initialize;
		fpUpdate = Level1_Update;
		fpDraw = Level1_Draw;
		fpFree = Level1_Free;
		fpUnload = Level1_Unload;
		break;
	case GS_LEVEL2: // Level 2 game state
		fpLoad = Level2_Load;
		fpInitialize = Level2_Initialize;
		fpUpdate = Level2_Update;
		fpDraw = Level2_Draw;
		fpFree = Level2_Free;
		fpUnload = Level2_Unload;
		break;
	case GS_RESTART: // State indicating the game should restart
		break;
	case GS_QUIT: // State indicating the game should quit/exit
		break;
	default: // Handle any undefined state IDs
		break;
	}

}
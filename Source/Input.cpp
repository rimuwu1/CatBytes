/* Start Header ************************************************************************/
/*!
\file Input.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Input Handling, which processes user inputs.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Input.h"
#include "AEEngine.h"

static Player* s_CurrentPlayer = nullptr;

void Input_SetPlayer(Player* player)
{
	s_CurrentPlayer = player;
}

// ----------------------------------------------------------------------------
// Handles all user input processing for the current frame
// This function should be called once per frame to process keyboard, mouse,
// or gamepad input and update game state accordingly
// ----------------------------------------------------------------------------
void Input_Handle() {
	// check if forcing the application to quit
	if (0 == AESysDoesWindowExist()) {
	    next = GS_QUIT;
    }

	// ESC goes to main menu
	if (current == GS_SPLASHSCREEN && AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = GS_MAINMENU;  
	}

	if(current == GS_LEVEL1 || current == GS_LEVEL2)
	{
		if (AEInputCheckTriggered(AEVK_ESCAPE)) {
			next = GS_MAINMENU;  
		}
	}

	// Q to quit the game
	if (AEInputCheckTriggered('Q')) {
		next = GS_QUIT;
	}

	// Process player movement input if a player is bound
	if (s_CurrentPlayer)
	{
		const float MOVE_SPEED = 400.0f;
		const float JUMP_FORCE = 650.0f;

		// Reset horizontal velocity each frame; input determines movement
		s_CurrentPlayer->vel.x = 0.0f;

		// Horizontal movement (A/D)
		if (AEInputCheckCurr('A')) {
			s_CurrentPlayer->vel.x -= MOVE_SPEED;
		}
		if (AEInputCheckCurr('D')) {
			s_CurrentPlayer->vel.x += MOVE_SPEED;
		}

		// Jumping (Space) - only when grounded
		if (s_CurrentPlayer->grounded && AEInputCheckCurr(AEVK_SPACE))
		{
			s_CurrentPlayer->vel.y = JUMP_FORCE;
			s_CurrentPlayer->grounded = 0;
		}
	}

	std::cout << "Input:Handle" << std::endl;
}
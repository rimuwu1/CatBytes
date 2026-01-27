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
#include "Utils.h"
#include <vector>

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

	// ESC goes back to main menu (from anywhere)
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		next = GS_MAINMENU;
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

		//Weapon equip / unequip toggle
	//press F to switch melee weapon on/off
		if (AEInputCheckTriggered(AEVK_F))
		{
			s_CurrentPlayer->weaponEquipped = !s_CurrentPlayer->weaponEquipped;

			if (s_CurrentPlayer->weaponEquipped)
			{
				s_CurrentPlayer->weapon = PlayerWeapon::MELEE;
			}
			else
			{
				s_CurrentPlayer->weapon = PlayerWeapon::NONE;
			}
		}


		// Melee attack (LMB)
		if (s_CurrentPlayer->weapon == PlayerWeapon::MELEE &&
			AEInputCheckTriggered(AEVK_LBUTTON))
		{
			s_CurrentPlayer->isAttacking = true;
			s_CurrentPlayer->attackTimer = 0.4f; //short attack window
		}
	}

	std::cout << "Input:Handle" << std::endl;
}
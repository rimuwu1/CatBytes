/* Start Header ************************************************************************/
/*!
\file Input.cpp
\author Joash ng, joash.ng, 2502780
		Tse Xuan Qi Tristin, tse.x, 2503757
\par joash.ng@digipen.edu
	 tse.x@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Input Handling, which processes global user inputs.

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
#include <vector>
#include "MainGame.h"
#include "FileManager.h"

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
		if(current == GS_SPLASHSCREEN)
		next = GS_MAINMENU;
		else if (current == GS_MAINGAME) {
			next = GS_PAUSE;
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

		//Weapon equip / unequip toggle
		//weapon switch: 1 = none, 2 = melee, 3 = gun
		if (AEInputCheckTriggered('1'))
		{
			s_CurrentPlayer->weapon = PlayerWeapon::NONE;
			s_CurrentPlayer->weaponEquipped = false;
			s_CurrentPlayer->isAttacking = false;
		}
		if (AEInputCheckTriggered('2'))
		{
			s_CurrentPlayer->weapon = PlayerWeapon::MELEE;
			s_CurrentPlayer->weaponEquipped = true;
			s_CurrentPlayer->isAttacking = false; // reset attack state
		}
		if (AEInputCheckTriggered('3'))
		{
			s_CurrentPlayer->weapon = PlayerWeapon::GUN;
			s_CurrentPlayer->weaponEquipped = true;
			s_CurrentPlayer->isAttacking = false; // melee attack off
		}


		if (AEInputCheckTriggered('L')) {
			MainGame_RequestSave();
		}
		if (AEInputCheckTriggered('M')) {
			GameSave::ResetSave(); //Debug feature for level 1!! remove when not needed
			GameSave::Notify_Show(GameSave::NotifyType::RESET);
		}
	}

	std::cout << "Input:Handle" << std::endl;
}
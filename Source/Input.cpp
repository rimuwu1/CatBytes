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
#include "EnvironmentManager.h"
#include "AudioManager.h"
#include "Audio.h"
#include "UIManager.h"
#include "ObjectManager.h"
#include "PhysicsManager.h"

// ----------------------------------------------------------------------------
// Handles all user input processing for the current frame
// This function should be called once per frame to process keyboard, mouse,
// or gamepad input and update game state accordingly
// ----------------------------------------------------------------------------
void Input_Handle() {
	// check if forcing the application to quit
	if (0 == AESysDoesWindowExist()) {
		GameStateManager::Get().next = GS_QUIT;
	}

	// ESC goes back to main menu (from anywhere)
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		if (GameStateManager::Get().current == GS_SPLASHSCREEN)
			GameStateManager::Get().next = GS_MAINMENU;
	}

	// Q to quit the game -- show confirmation popup
	if (AEInputCheckTriggered('Q')) {
		UIManager::Get().ShowConfirmation(
			"Quit Game",
			"Are you sure you want to quit?",
			[]() { GameStateManager::Get().next = GS_QUIT; },
			[]() {} // cancel: do nothing
		);
	}

	// Process player movement input
	{
		Player& player = ObjectManager::Get().GetPlayer();
		PhysicsManager& physics = PhysicsManager::Get();
		const float MOVE_SPEED = PhysicsManager::Get().GetMoveSpeed();

		// Horizontal velocity is fully input-driven each frame (no drag/friction needed)
		const bool moveLeft = AEInputCheckCurr('A') != 0;
		const bool moveRight = AEInputCheckCurr('D') != 0;
		player.vel.x = physics.ComputeHorizontalVelocity(moveLeft, moveRight, MOVE_SPEED);

		// Jumping (Space) — PhysicsManager validates grounded state internally
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			bool grounded = static_cast<bool>(player.grounded);
			if (physics.TryJump(player.vel.y, grounded))
			{
				player.grounded = grounded ? 1 : 0;
			}
		}

		//Weapon equip / unequip toggle
		//weapon switch: 1 = none, 2 = melee, 3 = gun
		if (AEInputCheckTriggered('1'))
		{
			player.weapon = PlayerWeapon::NONE;
			player.weaponEquipped = false;
			player.isAttacking = false;
		}
		if (AEInputCheckTriggered('2'))
		{
			player.weapon = PlayerWeapon::MELEE;
			player.weaponEquipped = true;
			player.isAttacking = false; // reset attack state
		}
		if (AEInputCheckTriggered('3'))
		{
			player.weapon = PlayerWeapon::GUN;
			player.weaponEquipped = true;
			player.isAttacking = false; // melee attack off
		}


		if (AEInputCheckTriggered('L')) {
			EnvironmentManager::Get().RequestSave();
		}
		if (AEInputCheckTriggered('M')) {
			GameSave::ResetSave(); //Debug feature for level 1!! remove when not needed
			GameSave::Notify_Show(GameSave::NotifyType::RESET);
		}
	}

	std::cout << "Input:Handle" << std::endl;
}
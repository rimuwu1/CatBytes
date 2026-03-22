/* Start Header ************************************************************************/
/*!
\file Input.cpp
\author Joash ng, joash.ng, 2502780
		Tse Xuan Qi Tristin, tse.x, 2503757
		Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par joash.ng@digipen.edu
	 tse.x@digipen.edu
	 kerwinjiajie.wong@digipen.edu
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
#include "GameSaveManager.h"
#include "EnvironmentManager.h"
#include "AudioManager.h"
#include "UIManager.h"
#include "ObjectManager.h"
#include "PhysicsManager.h"
#include "Camera.h"

// ----------------------------------------------------------------------------
// Handles all user input processing for the current frame
// This function should be called once per frame to process keyboard, mouse,
// or gamepad input and update game state accordingly
// ----------------------------------------------------------------------------
void Input_Handle() {
	float dt = (float)AEFrameRateControllerGetFrameTime();
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
		HUD& hud = EnvironmentManager::Get().GetHUD();
		PhysicsManager& physics = PhysicsManager::Get();
		const float MOVE_SPEED = PhysicsManager::Get().GetMoveSpeed();

		// Knockback: override normal movement
		if (player.knockbackTimer > 0.0f)
		{
			player.knockbackTimer -= dt;
			if (player.knockbackTimer <= 0.0f)
			{
				player.knockbackTimer = 0.0f;
				player.knockbackVel = { 0.0f, 0.0f };
			}
			else
			{
				player.vel.x = player.knockbackVel.x;
				player.vel.y = player.knockbackVel.y;
			}
		}
		else
		{
			// Horizontal velocity is fully input-driven each frame (no drag/friction needed)
			const bool moveLeft = AEInputCheckCurr('A') != 0;
			const bool moveRight = AEInputCheckCurr('D') != 0;
			player.vel.x = physics.ComputeHorizontalVelocity(moveLeft, moveRight, MOVE_SPEED);
		}

		// Jumping (Space) � PhysicsManager validates grounded state internally
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

		// Check for weapon slot clicks (alternative to keyboard)
		PlayerWeapon clickedWeapon = hud.IsWeaponSlotClicked(globalCam.x, globalCam.y);
		if (clickedWeapon != PlayerWeapon::NONE)
		{
			// Trigger press animation for the clicked slot
			int slotIndex = (clickedWeapon == PlayerWeapon::MELEE) ? 0 : 1;
			hud.TriggerWeaponSlotPress(slotIndex);

			player.weapon = clickedWeapon;
			player.weaponEquipped = true;
			player.isAttacking = false;
		}

		// Inventory slots
		// 4 = slot 1, 5 = slot 2, 6 = slot 3
		if (AEInputCheckTriggered('4') || AEInputCheckTriggered('5') || AEInputCheckTriggered('6'))
		{
			int slot;
			if (AEInputCheckTriggered('4')) slot = 0;
			else if (AEInputCheckTriggered('5')) slot = 1;
			else slot = 2;

			EnvironmentManager::Get().GetHUD().UseBuffFromInventory(player, slot);
		}

		// Check for inventory slot clicks (alternative to keyboard)
		int clickedSlot = hud.IsInventorySlotClicked(globalCam.x, globalCam.y);
		if (clickedSlot >= 0)
		{
			// Trigger press animation for the clicked slot
			hud.TriggerInventorySlotPress(clickedSlot);

			hud.UseBuffFromInventory(player, clickedSlot);
		}

		// Dash buff
		// right click to dash when buff is enabled
		if (AEInputCheckTriggered(AEVK_RBUTTON))
		{
			if (player.dashEnabled && !player.isDashing && player.dashCooldown <= 0.0f)
			{
				player.isDashing = true;
				player.dashTimer = Player::DASH_DURATION;
				player.vel.x = (player.facingRight ? 1.0f : -1.0f) * player.dashSpeed;
				player.vel.y = 0.0f;

				// decrement dash charges
				player.dashCharges--;
				if (player.dashCharges <= 0)
				{
					player.dashCharges = 0;
					player.dashEnabled = false;
				}
			}
		}
	}

	std::cout << "Input:Handle" << std::endl;
}
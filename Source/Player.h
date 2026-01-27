/* Start Header ************************************************************************/
/*!
\file Player.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinajijie.wong@digipen.edu
\date January, 23, 2026
\brief This file contains the function declarations for the Player movements, physics,
		input handling, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"

// ----------------------------------------------------------------------------
// Types of weapons the player can equip
// easy to extend later (gun, Sword, etc.)
// ----------------------------------------------------------------------------
enum class PlayerWeapon
{
	NONE = 0,//No weapon equipped(basic player)
	MELEE//Basic melee weapon
};

struct Player
{
	AEVec2 pos;
	AEVec2 vel;
	float width;
	float height;
	int grounded;

	float hp; //current player health(loaded from file)
	float hitTextTimer = 0.0f;//got hit

	//Weapon system (extendable)
	PlayerWeapon weapon;//which weapon is equipped
	bool weaponEquipped;//quick toggle flag

 //Melee attack state
	bool isAttacking;//true while melee swing is active
	float attackTimer;//how long the attack lasts
	float meleeDamage;//damage dealt to enemies

};


void Player_Init(Player& player, float startX, float startY);
void Player_Update(Player& player, float dt);
void Player_Draw(const Player& player);
void Player_Free();
void Player_ApplyDamage(Player& player, float damage);

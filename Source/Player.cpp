/* Start Header ************************************************************************/
/*!
\file Player.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinajijie.wong@digipen.edu
\date January, 23, 2026
\brief This file contains the function definitions for the Player movements, physics,
		input handling, and rendering.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "Player.h"
#include "Utils.h"
#include "Fonts.h"
#include <fstream>
#include "GameStateManager.h"//for mainmenu test

//small helper to load a  float from a text file(For Player HP)
static float LoadFloatFromFile(const char* filename)
{
	std::ifstream file(filename);
	float value = 0.0f;

	if (file.is_open())
		file >> value;//reads the first number

	return value;
}

//loads melee damage from file (Player melee attack)
static float LoadMeleeDamage()
{
	float dmg = LoadFloatFromFile("Assets/Data/PlayerDamageMelee.txt");
	return (dmg > 0.0f)? dmg:5.0f; //fallback to 5 ifnone
}

static AEGfxVertexList* playerMesh = nullptr;
static AEGfxTexture* playerTexture = nullptr;//no weapon
static AEGfxTexture* playerMeleeTexture = nullptr;//melee equipped
static AEGfxTexture* playerMeleeAttackTexture = nullptr;


void Player_Init(Player& player, float startX, float startY)
{
	player.pos.x = startX;
	player.pos.y = startY;
	player.vel.x = 0.0f;
	player.vel.y = 0.0f;
	player.width = 80.0f;
	player.height = 80.0f;
	player.grounded = 1;

	//load player hp from file, fallback to 5.0f if file fails
	player.hp = LoadFloatFromFile("Assets/Data/PlayerHP.txt");
	if (player.hp <= 0.0f)
		player.hp = 5.0f;//default hp

	// Weapon state
	player.weapon = PlayerWeapon::NONE;
	player.weaponEquipped = false;

	if (!playerMesh) // Check for player mesh
	{
		playerMesh = util::CreateSquareMesh();
	}

	//melee attack state
	player.isAttacking = false;
	player.attackTimer = 0.0f;
	player.meleeDamage = LoadMeleeDamage();

	//load player textures(shared)
	if (!playerTexture)
	{
		playerTexture = AEGfxTextureLoad("Assets/Images/player.jpg");
	}

	if (!playerMeleeTexture)
	{
		playerMeleeTexture = AEGfxTextureLoad("Assets/Images/PlayerMelee.jpg");
	}

	if (!playerMeleeAttackTexture)
	{
		playerMeleeAttackTexture =
			AEGfxTextureLoad("Assets/Images/PlayerMeleeAttack.jpg");
	}

}

void Player_Update(Player& player, float dt)
{
	const float GRAVITY = -1200.0f;

	// Gravity
	player.vel.y += GRAVITY * dt;


	//Weapon equip / unequip toggle
	//press F to switch melee weapon on/off
	if (AEInputCheckTriggered(AEVK_F))
	{
		player.weaponEquipped = !player.weaponEquipped;

		if (player.weaponEquipped)
		{
			player.weapon = PlayerWeapon::MELEE;
		}
		else
		{
			player.weapon = PlayerWeapon::NONE;
		}
	}


	// Melee attack (LMB)
	if (player.weapon == PlayerWeapon::MELEE &&
		AEInputCheckTriggered(AEVK_LBUTTON))
	{
		player.isAttacking = true;
		player.attackTimer = 0.4f; //short attack window
	}


	// Integrate velocity to position
	player.pos.x += player.vel.x * dt;
	player.pos.y += player.vel.y * dt;

	//For hit text
	//if (player.hitTextTimer > 0.0f)
		//player.hitTextTimer -= dt;

		//count down melee attack timer
	if (player.isAttacking)
	{
		player.attackTimer -= dt;
		if (player.attackTimer <= 0.0f)
		{
			player.attackTimer = 0.0f;
			player.isAttacking = false;
		}
	}

}

void Player_Draw(const Player& player)
{
	/*util::DrawSquare(
		playerMesh,
		player.pos.x,
		player.pos.y,
		player.width,
		player.height,
		0, 0, 255
	);*/

	//test for drawing image instead

	//draw the player using a textured square
	util::DrawTexturedSquare(
		playerMesh,
		(player.isAttacking) ? playerMeleeAttackTexture :
		(player.weapon == PlayerWeapon::MELEE ? playerMeleeTexture
			: playerTexture),
		player.pos.x,
		player.pos.y,
		player.width,
		player.height,
		1.0f//fully opaque
	);

	//TODO, will try to make it work another time
	/*
	  //draw hit text above the player
	if (player.hitTextTimer > 0.0f && g_FontSmall != -1)
	{
		float textX = player.pos.x;//same X as player
		float textY = player.pos.y + player.height * 0.5f + 20.0f; //above player

		AEGfxPrint(g_FontSmall, "Hit!", textX, textY,
			1.0f, 1.0f, 1.0f, 0.0f, 0.0f); //red
	}
	*/


}

//Apply damage to the player
 void Player_ApplyDamage(Player& player, float damage)
{
	if (player.hp <= 0.0f)
		return;

	player.hp -= damage;

	//show hit text for 0.5 seconds
	//player.hitTextTimer = 0.5f;

	//TEMPORARY death condition
	if (player.hp <= 0.0f)
	{
		player.hp = 0.0f;
		//trigger death: return to main menu
		next = GS_MAINMENU;
	}
}


// ----------------------------------------------------------------------------
// Releases all dynamically allocated resources used by the player
// this includes the player's mesh and texture, which are shared static resources
// ----------------------------------------------------------------------------
void Player_Free()
{
	//unload the player texture if it was loaded
	if (playerTexture)
	{
		AEGfxTextureUnload(playerTexture);
		playerTexture = nullptr; //prevent accidental reuse 
	}

	//free the player mesh if it exists
	if (playerMesh)
	{
		AEGfxMeshFree(playerMesh);
		playerMesh = nullptr;//mark as freed
	}

	if (playerMeleeTexture)
	{
		AEGfxTextureUnload(playerMeleeTexture);
		playerMeleeTexture = nullptr;
	}
}

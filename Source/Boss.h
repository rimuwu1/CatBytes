/* Start Header ************************************************************************/
/*!
\file       Level4.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
			Sim Hui Min, Huimin, s.huimin, 2503506
\par        p.yuxuanlovette@digipen.edu
			s.huimin@digipen.edu
\date       January 24 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "enemy.h"

// ----------------------------------------------------------------------------
// Boss level lifecycle (called by the game state manager)
// ----------------------------------------------------------------------------
void Boss_Load();
void Boss_Initialize();
void Boss_Update();
void Boss_Draw();
void Boss_Free();
void Boss_Unload();

// ----------------------------------------------------------------------------
// Pogba boss boss boss pogba enemy functions
// ----------------------------------------------------------------------------

// initialize the boss enemy at a given position.
// load HP, speed & damage
void BossEnemy_Init(Enemy& enemy, float startX, float startY);

// update the boss enemy each frame.
// patrols left/right 
void BossEnemy_Update(Enemy& enemy, float dt);

// draw the boss enemy on screen.
void BossEnemy_Draw(const Enemy& enemy);

// free boss-specific resources
void BossEnemy_Free();

// boss collision damage
void BossEnemy_OnCollision(Enemy& enemy, Player& player);

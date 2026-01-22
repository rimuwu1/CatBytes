/* Start Header ************************************************************************/
/*!
\file Level2.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements the functions for Level 2 of the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Level2.h"

// Global variables for tracking Level 2 game state
int Level2_Counter;  // Tracks remaining updates before checking lives
int Level2_Lives;    // Tracks remaining player lives in Level 2

// ----------------------------------------------------------------------------
// Loads Level 2 resources and initial data
// Reads the initial number of lives from a text file
// ----------------------------------------------------------------------------
void Level2_Load()
{
	// Log that loading is complete
	std::cout << "Level2:Load" << std::endl;
}

// ----------------------------------------------------------------------------
// Initializes Level 2 game state
// Called after loading to set up initial level conditions
// ----------------------------------------------------------------------------
void Level2_Initialize()
{
	// Log that initialization is complete
	std::cout << "Level2:Initialize" << std::endl;
}

// ----------------------------------------------------------------------------
// Updates Level 2 logic every frame
// Decrements the counter and manages lives/level progression
// ----------------------------------------------------------------------------
void Level2_Update()
{
	std::cout << "Level2:Update" << std::endl;
}

// ----------------------------------------------------------------------------
// Renders Level 2 graphics every frame
// Called after Update to draw the current game state
// ----------------------------------------------------------------------------
void Level2_Draw()
{
	std::cout << "Level2:Draw" << std::endl;
}

// ----------------------------------------------------------------------------
// Frees dynamic resources used by Level 2
// Called before unloading to release runtime-allocated resources
// ----------------------------------------------------------------------------
void Level2_Free()
{
	std::cout << "Level2:Free" << std::endl;
}

// ----------------------------------------------------------------------------
// Unloads Level 2 persistent resources
// Called when level is completely finished to clean up loaded assets
// ----------------------------------------------------------------------------
void Level2_Unload()
{
	std::cout << "Level2:Unload" << std::endl;
}
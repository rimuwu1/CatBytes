/* Start Header ************************************************************************/
/*!
\file GameStateList.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file contains the enumeration list for different game states.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

// ----------------------------------------------------------------------------
// Enumeration of all possible game states
// Used to identify and transition between different game states
// ----------------------------------------------------------------------------
enum GS_STATES
{
	GS_SPLASHSCREEN, // Splash Screen state
	GS_MAINMENU,    // Main Menu state
	GS_LEVEL1,		// Level 1 game state 
	GS_LEVEL2,      // Level 2 game state 
	GS_QUIT,        // Quit state to exit the game
	GS_RESTART      // Restart state to reload current level 
};
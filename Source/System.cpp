/* Start Header ************************************************************************/
/*!
\file System.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file implements system initialization and cleanup functions.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Fonts.h" 

// ----------------------------------------------------------------------------
// Initializes the game system and its subsystems
// Called once at the start of the game to set up the application
// ----------------------------------------------------------------------------
void System_Initialize(HINSTANCE hInstance, int nCmdShow)
{
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);
	AESysSetWindowTitle("CatBytes Game");
	AESysReset();

	Fonts_Load();

	std::cout << "System:Initialize" << std::endl;  // Log system initialization
}

// ----------------------------------------------------------------------------
// Cleans up and exits the game system
// Called once at the end of the game to perform cleanup operations
// ----------------------------------------------------------------------------
void System_Exit()
{
	Fonts_Unload();

	AESysExit();
	std::cout << "System:Exit" << std::endl;  // Log system exit/cleanup
}
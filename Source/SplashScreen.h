/* Start Header ************************************************************************/
/*!
\file SplashScreen.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file declares the credit sequence in the beginning of gamestate.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
void SplashScreen_Load();
void SplashScreen_Initialize();
void SplashScreen_Update();
void SplashScreen_Draw();
void SplashScreen_Free();
void SplashScreen_Unload();

// Skip the cutscene and jump to loading phase
void SplashScreen_SkipCutscene();

// Returns true if cutscene is still playing (before loading phase)
bool SplashScreen_IsInCutscene();
/* Start Header ************************************************************************/
/*!
\file       MainGame.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       February 4 2026
\brief		This file contains the function declarations for the main game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

void MainGame_Load();
void MainGame_Initialize();
void MainGame_Update();
void MainGame_Draw();
void MainGame_Free();
void MainGame_Unload();
void MainGame_RequestSave();

// ------------------------------------------------------------------------
// Splash screen loading API - runs during splash screen cutscene
// Phase 1: JSON parsing in background thread (StartAsyncLoading)
// Phase 2: Manager loading on main thread (ContinueMainThreadLoading)
// ------------------------------------------------------------------------
void ResetGameDataLoaded();
bool IsGameDataLoaded();

// Start async JSON parsing - spawns background thread (call from SplashScreen_Initialize)
void StartAsyncLoading();

// Continue main-thread loading - call each frame from SplashScreen_Update
// Returns true when all loading is complete
bool ContinueMainThreadLoading();

// Get loading progress (0-100) for display
int GetLoadingProgress();

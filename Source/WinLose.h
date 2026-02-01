#pragma once
/* Start Header *****/
/*!
\file       WinLose.h
\author     Sim Hui Min, Huimin, s.huimin, 2503506
\par        s.huimin@digipen.edu
\date       February 01 2026
\brief      Declares the Game Over screen state functions.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#pragma once

extern const char* textScreenMessage; // winlose text

void WinLose_Load();
void WinLose_Initialize();
void WinLose_Update();
void WinLose_Draw();
void WinLose_Free();
void WinLose_Unload();
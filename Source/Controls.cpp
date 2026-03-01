/* Start Header ************************************************************************/
/*!
\file Controls.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief 

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "Controls.h"
#include "GameStateManager.h"

void Controls_Load()
{
    std::cout << "Controls:Load" << std::endl;
}

void Controls_Initialize()
{
    std::cout << "Controls:Initialize" << std::endl;
}

void Controls_Update()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        GameStateManager::Get().next = GS_MAINMENU;
    }
}

void Controls_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.0f, 0.0f, 1.0f); // BLUE
    AESysFrameEnd();
}

void Controls_Free()
{
    std::cout << "Controls:Free" << std::endl;
}

void Controls_Unload()
{
    std::cout << "Controls:Unload" << std::endl;
}
/* Start Header ************************************************************************/
/*!
\file Credits.h
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
#include "Credits.h"
#include "GameStateManager.h"

void Credits_Load()
{
    std::cout << "Credits:Load" << std::endl;
}

void Credits_Initialize()
{
    std::cout << "Credits:Initialize" << std::endl;
}

void Credits_Update()
{
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        GameStateManager::Get().next = GS_MAINMENU;
    }
}

void Credits_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(1.0f, 0.0f, 1.0f); // PINK
    AESysFrameEnd();
}

void Credits_Free()
{
    std::cout << "Credits:Free" << std::endl;
}

void Credits_Unload()
{
    std::cout << "Credits:Unload" << std::endl;
}
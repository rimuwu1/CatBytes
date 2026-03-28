/* Start Header ************************************************************************/
/*!
\file Credits.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Sim Hui Min, s.huimin, 2503506
\par tse.x@digipen.edu
     s.huimin@digipen.edu
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
#include "Fonts.h"
#include "MeshManager.h"
#include "TextureManager.h"

static float totalElapsedTime = 0.0f;
static AEGfxTexture* logoTexture = nullptr;

void Credits_Load()
{
    std::cout << "Credits:Load" << std::endl;
}

void Credits_Initialize()
{
    std::cout << "Credits:Initialize" << std::endl;
    totalElapsedTime = 0.0f;

    // logo texture
    logoTexture = TextureManager::Get().LoadTexture("Assets/Images/titleicon.png");  
}

void Credits_Update()
{
    float dt = static_cast<float>(AEFrameRateControllerGetFrameTime());
    totalElapsedTime += dt;

    // skip with ESC
    if (totalElapsedTime > 5.0f || AEInputCheckTriggered(AEVK_ESCAPE))
    {
        GameStateManager::Get().next = GS_MAINMENU;
        totalElapsedTime = 0.0f;
    }
}
void Credits_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.9f, 0.9f, 0.9f); // light gray background

    // screen 1 logo
    if (totalElapsedTime <= 1.0f)
    {
        if (logoTexture)
        {
            // Draw logo using MeshManager
            MeshManager::Get().DrawTexturedSquare(logoTexture, 0.0f, 0.0f, 500.0f, 300.0f, 1.0f);
        }
    }

    // screen 2 team members (1-2 seconds)
     else if (totalElapsedTime > 1.0f && totalElapsedTime <= 2.0f)
     {
         AEGfxSetBlendMode(AE_GFX_BM_BLEND);
         FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "CatBytes", 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Peh Yu Xuan, Lovette", 0.0f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Tse Xuan Qi Tristin", 0.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Sim Hui Min", 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Joash Ng", 0.0f, -0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Kerwin Wong Jia Jie", 0.0f, -0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
     }
     // screen 3 faculty and advisors (2-3 seconds)
     else if (totalElapsedTime > 2.0f && totalElapsedTime <= 3.0f)
     {
         AEGfxSetBlendMode(AE_GFX_BM_BLEND);
         FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "Faculty and Advisors", 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(), "Instructors", 0.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Gerald Wong Han Feng", 0.0f, 0.05f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Dr. Soroor Malekmohammadai Faradounbeh", 0.0f, -0.05f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Tommy Tan Chee Wei", 0.0f, -0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
     }
     // screen 4 digipen credits (3-4 seconds)
     else if (totalElapsedTime > 3.0f && totalElapsedTime <= 4.0f)
     {
         AEGfxSetBlendMode(AE_GFX_BM_BLEND);
         FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "Created at", 0.0f, 0.55f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "DigiPen Institute of Technology Singapore", 0.0f, 0.40f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

         FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(), "PRESIDENT", 0.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Claude Comair", 0.0f, 0.08f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

         FontManager::Get().PrintCentered(FontManager::Get().GetMediumFont(), "EXECUTIVES", 0.0f, -0.15f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Jason Chu  Samir Abou Samra  Michele Comair", 0.0f, -0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Angela Kugler  Erik Mohrmann  Benjamin Ellinger", 0.0f, -0.33f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Melvin Gonsalvez  Michael GATS  TAN Chek Ming", 0.0f, -0.41f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Prasanna Kumar GHALI  Mandy WONG  Johnny DEEK", 0.0f, -0.49f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "All content (c) 2025 DigiPen Institute of Technology Singapore.", 0.0f, -0.80f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "All Rights Reserved.", 0.0f, -0.85f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
     }
     // screen 5: asset credits (4-5 seconds)
     else if (totalElapsedTime > 4.0f && totalElapsedTime <= 5.0f)
     {
         AEGfxSetBlendMode(AE_GFX_BM_BLEND);
         FontManager::Get().PrintCentered(FontManager::Get().GetLargeFont(), "Credits", 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "LibreSprite - libresprite.github.io", 0.0f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
         FontManager::Get().PrintCentered(FontManager::Get().GetSmallFont(), "Resprite - Jiaxing Fengeon Software", 0.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
     }

    AESysFrameEnd();
}

void Credits_Free()
{
    std::cout << "Credits:Free" << std::endl;
    totalElapsedTime = 0.0f;
}

void Credits_Unload()
{
    std::cout << "Credits:Unload" << std::endl;
    logoTexture = nullptr;
}
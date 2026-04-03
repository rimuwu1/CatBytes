/* Start Header ************************************************************************/
/*!
\file main.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Main entry point for game, implements the game loop which handles state switching,
       preservation and restarting. Uses the GameStateManager singleton.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "GameStateManager.h"
#include "Input.h"
#include "System.h"

// ----------------------------------------------------------------------------
// Main entry point for the application
// ----------------------------------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize system components (console on if debug, else off)
    System_Initialize(hInstance, nCmdShow, false); //toggled off cos the rubrics say no console

    auto& gsm = GameStateManager::Get();
    gsm.Initialize(gsm.current);

    // Main game loop - runs until quit state is reached
    while (gsm.current != GS_QUIT)
    {
        // ----- ENTERING A STATE -----
        if (gsm.current != GS_RESTART)
        {
            gsm.Update();
            gsm.fpLoad();
        }
        else
        {
            gsm.next = gsm.previous;
            gsm.current = gsm.previous;
        }
        gsm.fpInitialize();

        // ----- STATE LOOP -----
        while (gsm.next == gsm.current)
        {
            Input_Handle();
            gsm.fpUpdate();
            gsm.fpDraw();
        }

        // ----- LEAVING A STATE -----
        gsm.fpFree();
        if (gsm.next != GS_RESTART)
            gsm.fpUnload();

        // ----- TRANSITION -----
        gsm.previous = gsm.current;
        gsm.current = gsm.next;
    }

    // Clean up system resources before exiting
    System_Exit();

    return 0;
}
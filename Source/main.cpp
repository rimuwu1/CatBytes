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

static int preservedState = -1;   // -1 means no state is preserved

// ----------------------------------------------------------------------------
// Main entry point for the application
// Implements the game state machine with main game loop
// ----------------------------------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize system components
    System_Initialize(hInstance, nCmdShow, true);

    auto& gsm = GameStateManager::Get();
    gsm.Initialize(gsm.current);

    // Main game loop - runs until quit state is reached
    while (gsm.current != GS_QUIT)
    {
        // ----- ENTERING A STATE -----
        bool enteringPreserved = (gsm.current == preservedState);
        if (!enteringPreserved)
        {
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
        }
        else
        {
            // State is loaded — just update function pointers
            gsm.Update();
            preservedState = -1;   // clear the flag
        }

        // ----- STATE LOOP -----
        while (gsm.next == gsm.current)
        {
            Input_Handle();
            gsm.fpUpdate();
            gsm.fpDraw();
        }

        // ----- LEAVING A STATE -----
        if (gsm.next == GS_PAUSE)
        {
            // Going to pause: keep the current state's data alive
            preservedState = gsm.current;
            // skip fpFree and fpUnload
        }
        else
        {
            gsm.fpFree();
            if (gsm.next != GS_RESTART)
                gsm.fpUnload();
        }

        // ----- TRANSITION -----
        gsm.previous = gsm.current;
        gsm.current = gsm.next;
    }

    // Clean up system resources before exiting
    System_Exit();

    return 0;
}
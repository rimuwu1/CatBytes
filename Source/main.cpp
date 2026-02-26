/* Start Header ************************************************************************/
/*!
\file main.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Main entry point for game, implements the game loop which handles state switching, preservation and restarting.

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

	GSM_Initialize(current);

	// Main game loop - runs until quit state is reached
    while (current != GS_QUIT)
    {
        // ----- ENTERING A STATE -----
        bool enteringPreserved = (current == preservedState);
        if (!enteringPreserved)
        {
            if (current != GS_RESTART)
            {
                GSM_Update();
                fpLoad();
            }
            else
            {
                next = previous;
                current = previous;
            }
            fpInitialize();
        }
        else
        {
            //state is loaded
            GSM_Update();
            preservedState = -1;   // clear the flag
        }

        // ----- STATE LOOP -----
        while (next == current)
        {
            Input_Handle();
            fpUpdate();
            fpDraw();
        }

        // ----- LEAVING A STATE -----
        if (next == GS_PAUSE)
        {
            // Going to pause: keep the current state’s data alive
            preservedState = current;
            // skip fpFree and fpUnload
        }
        else
        {
            fpFree();
            if (next != GS_RESTART)
                fpUnload();
        }

        // ----- TRANSITION -----
        previous = current;
        current = next;
    }

	// Clean up system resources before exiting
	System_Exit();

	return 0;
}


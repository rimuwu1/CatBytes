#include "pch.h"
#include "GameStateManager.h"
#include "Input.h"
#include "System.h"

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
	System_Initialize(hInstance, nCmdShow);

	GSM_Initialize(current);

	// Main game loop - runs until quit state is reached
	while (current != GS_QUIT)
	{
		// If not restarting, set up and load new state
		if (current != GS_RESTART) {
			GSM_Update();   // Update GSM to set function pointers for current state
			fpLoad();       // Load resources for the current state
		}
		else {
			// Restart logic: revert to previous state
			next = previous;    // Set next to the state we were in before restart
			current = previous; // Also update current to the previous state
		}

		// Initialize the current game state
		fpInitialize();

		// State loop - runs while staying in the same game state
		while (next == current) {
			Input_Handle();     // Process user input for this frame
			fpUpdate();         // Update game logic for current state
			fpDraw();           // Render current game state
		}

		// Clean up current state before transitioning
		fpFree();               // Free dynamic resources

		// Unload persistent resources unless restarting
		if (next != GS_RESTART) {
			fpUnload();         // Unload static resources
		}

		// Update state tracking for next iteration
		previous = current;     // Store current state as previous
		current = next;         // Transition to next state
	}

	// Clean up system resources before exiting
	System_Exit();

	return 0;
}


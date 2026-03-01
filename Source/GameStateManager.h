/* Start Header ************************************************************************/
/*!
\file GameStateManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 21/01/2026
\brief This file contains the GameStateManager class declaration, implemented
       as a Meyers singleton for managing game state transitions and function pointers.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

typedef void(*FP)(void);

class GameStateManager
{
public:
    // Returns the single instance of the GameStateManager
    static GameStateManager& Get()
    {
        static GameStateManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;

    void Initialize(int startingState);
    void Update();

    // State tracking
    int current{ 0 };
    int previous{ 0 };
    int next{ 0 };

    // State function pointers
    FP fpLoad{ nullptr };
    FP fpInitialize{ nullptr };
    FP fpUpdate{ nullptr };
    FP fpDraw{ nullptr };
    FP fpFree{ nullptr };
    FP fpUnload{ nullptr };

private:
    GameStateManager() = default;
    ~GameStateManager() = default;
};
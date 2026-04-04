/* Start Header ************************************************************************/
/*!
\file TransitionManager.h
\author Joash ng, joash.ng, 2502780
\par    joash.ng@digipen.edu
\date   04/04/2026
\brief This file contains the TransitionManager class declaration for handling
       full-screen cyberpunk strip transitions between game states.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

enum class TransitionPhase { Idle, Covering, Holding, Revealing };

class TransitionManager {
public:
    // Returns the single instance (Meyer's singleton) - matches codebase pattern
    static TransitionManager& Get()
    {
        static TransitionManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    TransitionManager(const TransitionManager&) = delete;
    TransitionManager& operator=(const TransitionManager&) = delete;

    void Update(float dt);
    void Draw(float camX = 0.0f, float camY = 0.0f);
    void Start(int nextStateId);
    bool IsActive() const;
    bool IsFullyCovered() const;  // True during Holding phase (screen fully covered)

private:
    TransitionManager() = default;
    ~TransitionManager() = default;

    static const int   STRIP_COUNT    = 14;
    static constexpr float STRIP_DURATION = 0.18f;
    static constexpr float STAGGER_TIME   = 0.04f;
    static constexpr float HOLD_DURATION  = 0.08f;

    TransitionPhase     m_phase        = TransitionPhase::Idle;
    float               m_elapsed      = 0.0f;
    int                 m_nextState    = -1;
    bool                m_stateChanged = false;
};

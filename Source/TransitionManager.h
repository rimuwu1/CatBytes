/* Start Header ************************************************************************/
/*!
\file TransitionManager.h
\author 
\par 
\date 
\brief This file contains the TransitionManager class declaration for handling
       full-screen cyberpunk strip transitions between game states.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"

enum class TransitionPhase { Idle, Covering, Holding, Revealing };

class TransitionManager {
public:
    static TransitionManager& GetInstance();

    void Init();    // call once at app startup or first use
    void Free();    // call once at app shutdown
    void Update(float dt);
    void Draw();
    void Start(int nextStateId);
    bool IsActive() const;

private:
    TransitionManager() = default;
    ~TransitionManager() = default;
    TransitionManager(const TransitionManager&) = delete;
    TransitionManager& operator=(const TransitionManager&) = delete;

    static const int   STRIP_COUNT    = 14;
    static const float STRIP_DURATION;
    static const float STAGGER_TIME;
    static const float HOLD_DURATION;

    TransitionPhase     m_phase        = TransitionPhase::Idle;
    float               m_elapsed      = 0.0f;
    int                 m_nextState    = -1;
    bool                m_stateChanged = false;
    AEGfxVertexList*    m_pMesh        = nullptr;
};
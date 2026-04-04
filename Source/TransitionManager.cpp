/* Start Header ************************************************************************/
/*!
\file TransitionManager.cpp
\author 
\par 
\date 
\brief This file implements the TransitionManager singleton for handling
       full-screen cyberpunk strip transitions between game states.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "TransitionManager.h"
#include "GameStateManager.h"
#include <cmath>

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const float TransitionManager::STRIP_DURATION = 0.18f;
const float TransitionManager::STAGGER_TIME   = 0.04f;
const float TransitionManager::HOLD_DURATION  = 0.08f;

// ----------------------------------------------------------------------------
// GetInstance - Meyer's singleton
// ----------------------------------------------------------------------------
TransitionManager& TransitionManager::GetInstance()
{
    static TransitionManager instance;
    return instance;
}

// ----------------------------------------------------------------------------
// Init - Create mesh if not already created
// ----------------------------------------------------------------------------
void TransitionManager::Init()
{
    if (m_pMesh != nullptr) return;

    AEGfxMeshStart();
    // Unit quad - will be scaled per strip
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
                 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
                -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd( 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
                 0.5f,  0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
                -0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    m_pMesh = AEGfxMeshEnd();
}

// ----------------------------------------------------------------------------
// Free - Release mesh resources
// ----------------------------------------------------------------------------
void TransitionManager::Free()
{
    if (m_pMesh != nullptr)
    {
        AEGfxMeshFree(m_pMesh);
        m_pMesh = nullptr;
    }
}

// ----------------------------------------------------------------------------
// Start - Begin transition to next state
// ----------------------------------------------------------------------------
void TransitionManager::Start(int nextStateId)
{
    if (m_phase != TransitionPhase::Idle) return;

    m_nextState    = nextStateId;
    m_phase        = TransitionPhase::Covering;
    m_elapsed      = 0.0f;
    m_stateChanged = false;
}

// ----------------------------------------------------------------------------
// IsActive - Check if transition is in progress
// ----------------------------------------------------------------------------
bool TransitionManager::IsActive() const
{
    return m_phase != TransitionPhase::Idle;
}

// ----------------------------------------------------------------------------
// Update - Progress through transition phases
// ----------------------------------------------------------------------------
void TransitionManager::Update(float dt)
{
    if (m_phase == TransitionPhase::Idle) return;

    m_elapsed += dt;
    float totalCoverTime = STRIP_DURATION + (STRIP_COUNT - 1) * STAGGER_TIME;

    if (m_phase == TransitionPhase::Covering)
    {
        if (m_elapsed >= totalCoverTime)
        {
            if (!m_stateChanged)
            {
                // Queue the state change via GameStateManager
                GameStateManager::Get().next = m_nextState;
                m_stateChanged = true;
            }
            m_phase   = TransitionPhase::Holding;
            m_elapsed = 0.0f;
        }
    }
    else if (m_phase == TransitionPhase::Holding)
    {
        if (m_elapsed >= HOLD_DURATION)
        {
            m_phase   = TransitionPhase::Revealing;
            m_elapsed = 0.0f;
        }
    }
    else if (m_phase == TransitionPhase::Revealing)
    {
        if (m_elapsed >= totalCoverTime)
        {
            m_phase = TransitionPhase::Idle;
        }
    }
}

// ----------------------------------------------------------------------------
// Draw - Render cyberpunk strip transition effect
// ----------------------------------------------------------------------------
void TransitionManager::Draw()
{
    if (m_phase == TransitionPhase::Idle) return;

    float screenWidth  = (float)AEGfxGetWindowWidth();
    float screenHeight = (float)AEGfxGetWindowHeight();
    float stripHeight = screenHeight / STRIP_COUNT;

    // Strip colors
    const float deepPurpleR = 0.18f, deepPurpleG = 0.0f,  deepPurpleB = 0.35f;
    const float cyanR        = 0.0f,  cyanG        = 0.81f, cyanB       = 1.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    float totalCoverTime = STRIP_DURATION + (STRIP_COUNT - 1) * STAGGER_TIME;

    for (int i = 0; i < STRIP_COUNT; ++i)
    {
        // Calculate animation progress for this strip
        float delay  = static_cast<float>(i) * STAGGER_TIME;
        float localT = (m_elapsed - delay) / STRIP_DURATION;
        localT       = std::max(0.0f, std::min(1.0f, localT));
        float easedT = 1.0f - powf(1.0f - localT, 3.0f);  // ease-out cubic

        // Calculate X offset based on phase and strip index
        float xOffset = 0.0f;
        if (m_phase == TransitionPhase::Covering)
        {
            // Even strips slide from left, odd from right
            if (i % 2 == 0)
            {
                xOffset = -screenWidth * (1.0f - easedT);
            }
            else
            {
                xOffset = screenWidth * (1.0f - easedT);
            }
        }
        else if (m_phase == TransitionPhase::Holding)
        {
            xOffset = 0.0f;  // All strips in final position
        }
        else if (m_phase == TransitionPhase::Revealing)
        {
            // Reverse directions
            if (i % 2 == 0)
            {
                xOffset = -screenWidth * easedT;
            }
            else
            {
                xOffset = screenWidth * easedT;
            }
        }

        // Calculate Y position for this strip
        // Screen space: Y increases upward, center is (0,0)
        // Strip i spans from bottom=i*stripHeight to top=(i+1)*stripHeight
        // In screen space: centerY = (screenHeight/2) - (i + 0.5f) * stripHeight
        float stripCentreY = (screenHeight * 0.5f) - (static_cast<float>(i) + 0.5f) * stripHeight;

        // Select color based on strip index
        float r, g, b;
        if (i % 2 == 0)
        {
            r = deepPurpleR; g = deepPurpleG; b = deepPurpleB;
        }
        else
        {
            r = cyanR; g = cyanG; b = cyanB;
        }
        AEGfxSetColorToMultiply(r, g, b, 1.0f);

        // Build transformation matrix
        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, screenWidth, stripHeight);
        AEMtx33Trans(&trans, xOffset, stripCentreY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(m_pMesh, AE_GFX_MDM_TRIANGLES);
    }
}
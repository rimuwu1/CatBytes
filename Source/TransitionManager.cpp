/* Start Header ************************************************************************/
/*!
\file TransitionManager.cpp
\author Joash ng, joash.ng, 2502780
\par    joash.ng@digipen.edu
\date   04/04/2026
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
#include "MeshManager.h"
#include <cmath>

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
// IsFullyCovered - Check if screen is fully covered (safe to change state)
// ----------------------------------------------------------------------------
bool TransitionManager::IsFullyCovered() const
{
    return m_phase == TransitionPhase::Holding || m_phase == TransitionPhase::Revealing;
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
            // screen is fully covered - change state 
            if (!m_stateChanged)
            {
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
// Draw - Render cyberpunk strip transition effect using MeshManager
// camX, camY: camera offset to draw in screen space
// ----------------------------------------------------------------------------
void TransitionManager::Draw(float camX, float camY)
{
    if (m_phase == TransitionPhase::Idle) return;

    float screenWidth  = static_cast<float>(AEGfxGetWindowWidth());
    float screenHeight = static_cast<float>(AEGfxGetWindowHeight());
    float stripHeight  = screenHeight / STRIP_COUNT;

    // Strip colors (RGB 0-255 for MeshManager::DrawSquare) - two dark deep purple shades
    const int darkPurple1R = 20,  darkPurple1G = 0,   darkPurple1B = 40;   // darker shade
    const int darkPurple2R = 40,   darkPurple2G = 0,   darkPurple2B = 60;   // slightly lighter

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
        // Screen space: Y=0 is center, positive up
        // Strip i: bottom strip (i=0) at bottom of screen
        float stripCenterY = (screenHeight * 0.5f) - (static_cast<float>(i) + 0.5f) * stripHeight;

        // Select color based on strip index (alternating dark purple shades)
        int r, g, b;
        if (i % 2 == 0)
        {
            r = darkPurple1R; g = darkPurple1G; b = darkPurple1B;
        }
        else
        {
            r = darkPurple2R; g = darkPurple2G; b = darkPurple2B;
        }

        // Draw strip using MeshManager, offset by camera to stay in screen space
        MeshManager::Get().DrawSquare(camX + xOffset, camY + stripCenterY, screenWidth, stripHeight, r, g, b, 1.0f);
    }
}

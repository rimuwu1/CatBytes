/* Start Header ************************************************************************/
/*!
\file PhysicsManager.cpp
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date 05/03/2026
\brief Implements the PhysicsManager singleton.  All physics constants live here
       so that every system (Player, Enemy, Projectile …) shares the same values
       and time-based integration logic.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PhysicsManager.h"
#include <algorithm>   // std::max / std::min

// ----------------------------------------------------------------------------
// Constructor — sets default physics constants
// ----------------------------------------------------------------------------
PhysicsManager::PhysicsManager()
    : m_Gravity{ -1200.0f }
    , m_JumpForce{ 650.0f }
    , m_MoveSpeed{ 400.0f }
    , m_TerminalVel{ 2000.0f }
{
}

// ----------------------------------------------------------------------------
// ApplyGravity
//   Adds gravitational acceleration to the vertical velocity component.
//   When the object is grounded we skip gravity entirely so that the
//   velocity does not accumulate a large negative value while standing.
// ----------------------------------------------------------------------------
void PhysicsManager::ApplyGravity(float& vel_y, bool grounded, float dt) const
{
    if (!grounded)
        vel_y += m_Gravity * dt;
}

// ----------------------------------------------------------------------------
// ClampFallSpeed
//   Prevents the vertical velocity from growing beyond terminal velocity.
//   The terminal velocity magnitude is always positive; velocity is clamped
//   so it cannot be more negative than -m_TerminalVel.
// ----------------------------------------------------------------------------
void PhysicsManager::ClampFallSpeed(float& vel_y) const
{
    if (vel_y < -m_TerminalVel)
        vel_y = -m_TerminalVel;
}

// ----------------------------------------------------------------------------
// Integrate
//   Classic Euler integration: pos += vel * dt
// ----------------------------------------------------------------------------
void PhysicsManager::Integrate(AEVec2& pos, const AEVec2& vel, float dt) const
{
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
}

// ----------------------------------------------------------------------------
// ComputeHorizontalVelocity
//   Converts left/right boolean flags into a signed velocity value.
// ----------------------------------------------------------------------------
float PhysicsManager::ComputeHorizontalVelocity(bool left, bool right, float speedOverride) const
{
    const float speed = (speedOverride > 0.f) ? speedOverride : m_MoveSpeed;

    float vel_x = 0.f;
    if (left)  vel_x -= speed;
    if (right) vel_x += speed;
    return vel_x;
}

// ----------------------------------------------------------------------------
// TryJump
//   Applies the jump force to vel_y only if the object is currently grounded.
//   Returns true when a jump was applied so the caller can trigger effects
//   (sound, animation, etc.).
// ----------------------------------------------------------------------------
bool PhysicsManager::TryJump(float& vel_y, bool& grounded) const
{
    if (!grounded)
        return false;

    vel_y = m_JumpForce;
    grounded = false;
    return true;
}
/* Start Header ************************************************************************/
/*!
\file PhysicsManager.h
\author     Joash ng, joash.ng, 2502780
\par        joash.ng@digipen.edu
\date 05/03/2026
\brief Declares the PhysicsManager singleton, which owns all physics constants and
       provides time-based integration helpers (gravity, movement, jump) used by
       every game object that needs 2-D physics.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include "pch.h"

// ----------------------------------------------------------------------------
// PhysicsManager
// ----------------------------------------------------------------------------
class PhysicsManager
{
public:
    // ---- Singleton access --------------------------------------------------
    static PhysicsManager& Get()
    {
        static PhysicsManager instance;
        return instance;
    }

    // Prevent copies / moves
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;
    PhysicsManager(PhysicsManager&&) = delete;
    PhysicsManager& operator=(PhysicsManager&&) = delete;

    // ---- Physics constants -------------------------------------------------
    float GetGravity()        const { return m_Gravity; }   // units/s²  (negative = downward)
    float GetJumpForce()      const { return m_JumpForce; }   // units/s   (positive = upward)
    float GetMoveSpeed()      const { return m_MoveSpeed; }   // units/s   (base walk speed)
    float GetTerminalVel()    const { return m_TerminalVel; }   // units/s   (max fall speed, positive magnitude)

    // Allow runtime tweaks (for DebugManager)
    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }
    void SetGravity(float g) { m_Gravity = g; }
    void SetJumpForce(float jf) { m_JumpForce = jf; }
    void SetTerminalVel(float tv) { m_TerminalVel = tv; }

    // ---- Integration helpers -----------------------------------------------

    // Apply gravity to a vertical velocity component for one frame.
    // vel_y    : current vertical velocity (modified in-place)
    // grounded : if true gravity is not applied (standing on solid ground)
    // dt       : delta-time in seconds
    void ApplyGravity(float& vel_y, bool grounded, float dt) const;

    // Clamp vertical velocity so the object cannot fall faster than terminal velocity.
    void ClampFallSpeed(float& vel_y) const;

    // Integrate velocity into position for one frame.
    // pos : position (modified in-place)
    // vel : velocity (read-only)
    // dt  : delta-time in seconds
    void Integrate(AEVec2& pos, const AEVec2& vel, float dt) const;

    // Compute the horizontal velocity from raw directional input flags.
    // left / right : whether the corresponding movement key is held
    // speedOverride: if > 0 this replaces the manager's base move speed
    // Returns the resulting horizontal velocity (positive = right).
    float ComputeHorizontalVelocity(bool left, bool right, float speedOverride = 0.f) const;

    // Attempt a jump.
    // vel_y    : vertical velocity (modified in-place if the jump is allowed)
    // grounded : true if the object is currently on the ground
    // Returns true if a jump was actually applied.
    bool TryJump(float& vel_y, bool& grounded) const;

private:
    // Private constructor — only called once by Get()
    PhysicsManager();

    float m_Gravity;      // -1300 units/s²
    float m_JumpForce;    //   650 units/s
    float m_MoveSpeed;    //   400 units/s
    float m_TerminalVel;  //  2000 units/s  (max fall magnitude) put here first cos wanna remove die off cam
};
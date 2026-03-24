/* Start Header ********************************************************/
/*!
\file   BossAI.h
\author Sim Hui Min, s.huimin, 2503506
\par    s.huimin@digipen.edu
\date   22/01/2026
\brief  Boss AI state machine declaration.
*/
/* End Header **********************************************************/

#pragma once
#include "enemy.h"

struct Player;

// -----------------------------------------------------------------------
// boss AI phases (overall fight progression)
// -----------------------------------------------------------------------
enum class BossPhase {
    Phase1,             // full HP to 50%
    PhaseTransition,    // hurtbetweenphase 3s, usewatch 3s, then usepc 30s (lasers only fire in this mid phase)
    Phase2,             // after usepc same attacks 
    Defeated            // hp <= 0
};

// -----------------------------------------------------------------------
// boss attack states
// -----------------------------------------------------------------------
enum class BossAttackState {
    Idle,               // attackidle — tracking player, choosing next move
    WalkToRange,        // walk toward/away from player to reach attack range
    DashWindup,         // lock-on position shown, attackidle briefly before dash
    DashAttack,         // dashing toward locked position
    Punch,              // attackpunch animation, brief hit window
    ChargeSlam,         // chargeslam animation ~3s
    Jump,               // jump animation, rising to peak
    SlamDown,           // slamdown mid-air falling
    SlamImpact,         // slamimpact on landing
    Blocking,           // block animation
    Hurt,               // hurt animation on taking damage
    HurtBetweenPhase,   // hurtbetweenphase 3s (invincible)
    UseWatch,           // usewatch 3s (invincible)
    UsePC,              // usepc 30s (lasers on, still invincible)
    FightOver,          // fightover anim 3s (cutscene bars back)
    FightOverDisappear  // fightoverdisappear then freeze
};

// -----------------------------------------------------------------------
// persistent AI data stored alongside the Enemy
// -----------------------------------------------------------------------
struct BossAIData {
    BossPhase       phase            = BossPhase::Phase1;
    BossAttackState attackState      = BossAttackState::Idle;

    float stateTimer     = 0.0f;  // general per-state countdown
    float idleTimer      = 0.0f;  // time spent in Idle before picking attack
    float consecutiveHits = 0;    // how many hits player has landed recently
    float consecutiveHitTimer = 0.0f; // resets after a window

    bool blockFacingLeft = false;

    // dash/slam lock-on
    float lockedTargetX  = 0.0f;
    float lockedTargetY  = 0.0f;
    bool slamTargetLocked = false;

    // between phases
    bool  phaseTransitionDone = false;
    bool  lasersEnabled       = false;
    int usePCHitsRemaining = 1; // hits to take before exiting UsePC 

    // defeat sequence
    bool  defeatStarted = false;
    bool done = false;

};

// -----------------------------------------------------------------------
// API
// -----------------------------------------------------------------------
void BossAI_Init    (BossAIData& ai);
void BossAI_Update  (BossAIData& ai, Enemy& boss, Player& player, float dt);
bool BossAI_IsInvincible (const BossAIData& ai);
bool BossAI_LasersActive (const BossAIData& ai);
bool BossAI_IsFightOver  (const BossAIData& ai);
bool BossAI_IsFrozen     (const BossAIData& ai); 
bool BossAI_TryBlock(BossAIData& ai, Enemy& boss, Player& player); // block
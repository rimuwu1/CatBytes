/* Start Header ********************************************************/
/*!
\file   BossAI.cpp
\author Sim Hui Min, s.huimin, 2503506
\par    s.huimin@digipen.edu
\date   22/01/2026
\brief  Boss AI state machine declaration.
*/
/* End Header **********************************************************/

#include "pch.h"
#include "BossAI.h"
#include "Player.h"
#include "MeshManager.h"
#include "Camera.h"
#include <cmath>
#include <cstdlib>

// -----------------------------------------------------------------------
// tuning constants
// -----------------------------------------------------------------------
static constexpr float DASH_WINDUP_TIME     = 0.6f;   // lock-on telegraph before dash
static constexpr float DASH_SPEED           = 1200.0f;
static constexpr float PUNCH_DURATION       = 0.5f;   // attackpunch hold time
static constexpr float IDLE_MIN_TIME        = 1.0f;   // min time in Idle before attack
static constexpr float IDLE_MAX_TIME        = 2.0f;
static constexpr float CLOSE_RANGE          = 200.0f; // dash triggers at this distance
static constexpr float SLAM_CHARGE_TIME     = 1.0f; // pre charge-slam attack timing
static constexpr float JUMP_RISE_TIME       = 0.5f;
static constexpr float JUMP_SPEED           = 600.0f;
static constexpr float SLAM_FALL_SPEED      = 1400.0f;
static constexpr float SLAM_IMPACT_TIME     = 0.6f;
static constexpr float HURTBETWEEN_TIME     = 3.0f;
static constexpr float USEWATCH_TIME        = 3.0f;
static constexpr float USEPC_TIME           = 30.0f;
static constexpr float FIGHT_OVER_TIME      = 3.0f;
static constexpr float BLOCK_CHANCE         = 0.30f;
static constexpr float MAX_CONSEC_HITS      = 5.0f;
static constexpr float CONSEC_HIT_WINDOW    = 4.0f;   // seconds before hit counter resets
static constexpr float PHASE2_HP_THRESHOLD  = 0.5f;   // 50%
static constexpr float HURT_DURATION        = 1.0f;  // hurt state invincibility duration
static constexpr float USEPC_INTERRUPT_STUN = 2.0f;  // stun duration when UsePC is interrupted

static constexpr float WALK_SPEED           = 150.0f;
static constexpr float PREFERRED_RANGE_MIN  = 150.0f;
static constexpr float PREFERRED_RANGE_MAX  = 400.0f;

// -----------------------------------------------------------------------
static float RandFloat() { return (float)rand() / (float)RAND_MAX; }

static void PlayIfDifferent(Enemy& boss, const char* clip)
{
    if (!boss.spriteSheet) return;
    if (boss.spriteSheet->GetCurrentClip() != clip)
        boss.spriteSheet->Play(clip);
}

static void FacePlayer(Enemy& boss, const Player& player)
{
    boss.facesLeft = (player.pos.x < boss.pos.x);
}

// -----------------------------------------------------------------------
void BossAI_Init(BossAIData& ai)
{
    ai = BossAIData{};
}

// -----------------------------------------------------------------------
bool BossAI_IsInvincible(const BossAIData& ai) // states where boss can;t be damaged
{
    return ai.attackState == BossAttackState::HurtBetweenPhase
        || ai.attackState == BossAttackState::UseWatch
        || ai.attackState == BossAttackState::Hurt
        || ai.attackState == BossAttackState::Blocking;
}

bool BossAI_LasersActive(const BossAIData& ai)
{
    return ai.lasersEnabled
        && ai.attackState == BossAttackState::UsePC;
}

bool BossAI_IsFightOver(const BossAIData& ai)
{
    return ai.attackState == BossAttackState::FightOver
        || ai.attackState == BossAttackState::FightOverDisappear;
}

bool BossAI_IsFrozen(const BossAIData& ai)
{
    return ai.defeatStarted
        && ai.attackState == BossAttackState::FightOverDisappear
        && ai.stateTimer <= 0.0f;
}

// -----------------------------------------------------------------------
// pick next attack
// -----------------------------------------------------------------------
static BossAttackState PickNextAttack(const BossAIData& ai, const Enemy& boss, const Player& player)
{
    (void)ai;
    float dist = fabsf(player.pos.x - boss.pos.x);

    // if player is landing too many hits, prefer jump-slam to create distance
    if (ai.consecutiveHits >= MAX_CONSEC_HITS)
        return BossAttackState::ChargeSlam;

    // random chance between dash-punch and charge-slam, biased by distance
    if (dist < CLOSE_RANGE)
        return (RandFloat() < 0.7f) ? BossAttackState::DashWindup : BossAttackState::ChargeSlam;
    else
        return (RandFloat() < 0.4f) ? BossAttackState::DashWindup : BossAttackState::ChargeSlam;
}

// -----------------------------------------------------------------------
void BossAI_Update(BossAIData& ai, Enemy& boss, Player& player, float dt)
{
    if (!boss.isAlive && !ai.defeatStarted) return;

    // defeat check
    if (boss.hitPoints <= 0.1f && !ai.defeatStarted)
    {
        ai.defeatStarted = true;
        ai.phase         = BossPhase::Defeated;
        ai.attackState   = BossAttackState::FightOver;
        ai.stateTimer    = 0.0f;
        ai.lasersEnabled = false;
        boss.vel.x       = 0.0f;
        boss.vel.y       = 0.0f;
        PlayIfDifferent(boss, "fightover");
    }

    boss.isInvincible = BossAI_IsInvincible(ai);

    if (!BossAI_IsFrozen(ai))
        ai.stateTimer += dt;

    // always face the player unless frozen or in specific anims
    if (ai.attackState != BossAttackState::FightOverDisappear
        && ai.attackState != BossAttackState::FightOver)
        FacePlayer(boss, player);

    //  consecutive hit window decay 
    ai.consecutiveHitTimer += dt;
    if (ai.consecutiveHitTimer >= CONSEC_HIT_WINDOW)
    {
        ai.consecutiveHits    = 0;
        ai.consecutiveHitTimer = 0.0f;
    }

    // phase transition check (phase1 -> phase transition at 50% HP) 
    if (ai.phase == BossPhase::Phase1
        && boss.hitPoints <= boss.maxHitPoints * PHASE2_HP_THRESHOLD
        && !ai.phaseTransitionDone)
    {
        ai.phase             = BossPhase::PhaseTransition;
        ai.attackState       = BossAttackState::HurtBetweenPhase;
        ai.stateTimer        = 0.0f;
        ai.lasersEnabled     = false;
        PlayIfDifferent(boss, "hurtbetweenphase");
    }

    // ================================================================
    // state machine
    // ================================================================
    switch (ai.attackState)
    {
        // idle
    case BossAttackState::Idle:
    {
        PlayIfDifferent(boss, "attackidle");

        float dist = player.pos.x - boss.pos.x;
        float absDist = fabsf(dist);

        // slowly walk toward preferred range while idling
        if (absDist > PREFERRED_RANGE_MAX)
        {
            PlayIfDifferent(boss, "walk");
            boss.vel.x = (dist > 0.0f ? 1.0f : -1.0f) * WALK_SPEED * 0.6f;
        }
        else if (absDist < PREFERRED_RANGE_MIN)
        {
            PlayIfDifferent(boss, "walk");
            boss.vel.x = (dist > 0.0f ? -1.0f : 1.0f) * WALK_SPEED * 0.5f;
        }
        else
        {
            // in range — do a small back-and-forth pace
            PlayIfDifferent(boss, "walk");
            float paceDir = (ai.stateTimer < 0.8f) ? 1.0f : -1.0f;
            boss.vel.x = paceDir * WALK_SPEED * 0.3f;
            if (ai.stateTimer >= 1.6f)
                ai.stateTimer = 0.0f; // loop the pace cycle
        }

        ai.idleTimer += dt;
        float idleTarget = IDLE_MIN_TIME + RandFloat() * (IDLE_MAX_TIME - IDLE_MIN_TIME);

        if (ai.idleTimer >= idleTarget)
        {
            ai.idleTimer   = 0.0f;
            ai.stateTimer  = 0.0f;
            boss.vel.x     = 0.0f;
            ai.attackState = PickNextAttack(ai, boss, player);
        }
        break;
    }
    // walking
    case BossAttackState::WalkToRange:
    {
        PlayIfDifferent(boss, "walk");
        float dist = player.pos.x - boss.pos.x;
        float absDist = fabsf(dist);

        if (absDist >= PREFERRED_RANGE_MIN && absDist <= PREFERRED_RANGE_MAX)
        {
            boss.vel.x     = 0.0f;
            ai.attackState = BossAttackState::Idle;
            ai.idleTimer   = 0.0f;
            ai.stateTimer  = 0.0f;
        }
        else
        {
            float dir = (absDist > PREFERRED_RANGE_MAX)
                ? (dist > 0.0f ? 1.0f : -1.0f)
                : (dist > 0.0f ? -1.0f : 1.0f);
            boss.vel.x = dir * WALK_SPEED;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::DashWindup:
    {
        PlayIfDifferent(boss, "attackidle"); // TODO: replace wiith dash sprite 
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= DASH_WINDUP_TIME)
        {
            // lock on player position
            ai.lockedTargetX = player.pos.x;
            ai.lockedTargetY = player.pos.y;
            ai.attackState   = BossAttackState::DashAttack;
            ai.stateTimer    = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::DashAttack:
    {
        PlayIfDifferent(boss, "attackidle"); // temp but should be pre-attack dash when added, then attack dash

        float dx   = ai.lockedTargetX - boss.pos.x;
        float dist = fabsf(dx);

        if (dist < 20.0f)
        {
            // reached target — punch
            boss.vel.x     = 0.0f;
            ai.attackState = BossAttackState::Punch;
            ai.stateTimer  = 0.0f;
            PlayIfDifferent(boss, "attackpunch");
        }
        else
        {
            boss.vel.x = (dx > 0.0f ? 1.0f : -1.0f) * DASH_SPEED;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::Punch:
    {
        PlayIfDifferent(boss, "attackpunch");
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= PUNCH_DURATION)
        {
            ai.attackState = BossAttackState::WalkToRange;
            ai.stateTimer  = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::ChargeSlam:
    {
        PlayIfDifferent(boss, "chargeslam");
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= SLAM_CHARGE_TIME)
        {
            ai.slamTargetLocked = false; // reset for SlamDown
            ai.attackState      = BossAttackState::Jump;
            ai.stateTimer       = 0.0f;
            PlayIfDifferent(boss, "jump");
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::Jump:
    {
        PlayIfDifferent(boss, "jump");
        boss.vel.y = JUMP_SPEED;
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= JUMP_RISE_TIME)
        {
            ai.attackState = BossAttackState::SlamDown;
            ai.stateTimer  = 0.0f;
            PlayIfDifferent(boss, "slamdown");
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::SlamDown:
    {
        PlayIfDifferent(boss, "slamdown");

        if (!ai.slamTargetLocked)
        {
            ai.lockedTargetX    = player.pos.x; // lock exactly at start of slam
            ai.slamTargetLocked = true;
        }

        float dx = ai.lockedTargetX - boss.pos.x;
        boss.vel.x = (fabsf(dx) > 10.0f) ? (dx > 0 ? 1.0f : -1.0f) * 800.0f : 0.0f;
        boss.vel.y = -SLAM_FALL_SPEED;

        if (boss.pos.y <= -325.0f + boss.height * 0.5f + 5.0f)
        {
            boss.pos.y     = -325.0f + boss.height * 0.5f;
            boss.vel.x     = 0.0f;
            boss.vel.y     = 0.0f;
            ai.attackState = BossAttackState::SlamImpact;
            ai.stateTimer  = 0.0f;
            PlayIfDifferent(boss, "slamimpact");
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::SlamImpact:
    {
        PlayIfDifferent(boss, "slamimpact");
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= SLAM_IMPACT_TIME)
        {
            ai.attackState = BossAttackState::WalkToRange;
            ai.stateTimer  = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::Blocking:
    {
        PlayIfDifferent(boss, "block");
        boss.vel.x = 0.0f;

        // block ends after animation (~5 frames * 0.1s)
        if (!boss.spriteSheet->IsPlaying())
        {
            ai.attackState = BossAttackState::Idle;
            ai.stateTimer  = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    case BossAttackState::Hurt:
    {
        PlayIfDifferent(boss, "hurt");
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= HURT_DURATION)
        {
            ai.attackState = BossAttackState::Idle;
            ai.stateTimer  = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    // phase transition states
    // ----------------------------------------------------------------
    case BossAttackState::HurtBetweenPhase: // it's a stun.. idk why i didn't think of that word for so long HELP
    {
        PlayIfDifferent(boss, "hurtbetweenphase");
        boss.vel.x = 0.0f;

        // UsePC interrupted uses shorter stun, normal phase transition uses full 3s
        float stunDuration = (ai.phase == BossPhase::PhaseTransition && ai.phaseTransitionDone)
            ? USEPC_INTERRUPT_STUN
            : HURTBETWEEN_TIME;

        if (ai.stateTimer >= stunDuration)
        {
            if (ai.phase == BossPhase::PhaseTransition && !ai.phaseTransitionDone)
            {
                ai.attackState = BossAttackState::UseWatch;
            }
            else
            {
                ai.phaseTransitionDone = true;
                ai.phase               = BossPhase::Phase2;
                ai.lasersEnabled       = false;
                ai.attackState         = BossAttackState::WalkToRange;
            }
            ai.stateTimer = 0.0f;
        }
        break;
    }

    case BossAttackState::UseWatch:
    {
        PlayIfDifferent(boss, "usewatch");
        boss.vel.x = 0.0f;

        if (ai.stateTimer >= USEWATCH_TIME)
        {
            ai.attackState = BossAttackState::UsePC;
            ai.stateTimer  = 0.0f;
            PlayIfDifferent(boss, "usepc");
        }
        break;
    }

    case BossAttackState::UsePC:
    {
        PlayIfDifferent(boss, "usepc");
        boss.vel.x     = 0.0f;
        ai.lasersEnabled = true;

        if (ai.stateTimer >= USEPC_TIME)
        {
            // transition complete — Phase 2 begins
            ai.phaseTransitionDone = true;
            ai.phase               = BossPhase::Phase2;
            ai.lasersEnabled       = false;
            ai.attackState         = BossAttackState::WalkToRange;
            ai.stateTimer          = 0.0f;
        }
        break;
    }

    // ----------------------------------------------------------------
    // defeat sequence (end of fight cutscene)
    // ----------------------------------------------------------------
    case BossAttackState::FightOver:
    {
        PlayIfDifferent(boss, "fightover");
        boss.vel.x = 0.0f;
        boss.vel.y = 0.0f;

        if (ai.stateTimer >= FIGHT_OVER_TIME)
        {
            ai.attackState = BossAttackState::FightOverDisappear;
            ai.stateTimer  = 0.0f;
            PlayIfDifferent(boss, "fightoverdisappear");
        }
        break;
    }

    case BossAttackState::FightOverDisappear:
    {
        boss.vel.x = 0.0f;
        boss.vel.y = 0.0f;
        // freeze when animation ends
        if (boss.spriteSheet && !boss.spriteSheet->IsPlaying())
            ai.stateTimer = -1.0f; // sentinel: frozen
        break;
    }

    default: break;
    } // end switch

      // apply velocity to position — BossEnemy_Update no longer does this
    boss.pos.x += boss.vel.x * dt;
    boss.pos.y += boss.vel.y * dt;

    // clamp to floor
    const float floorY = -325.0f + boss.height * 0.5f;
    if (boss.pos.y < floorY)
    {
        boss.pos.y = floorY;
        boss.vel.y = 0.0f;
    }

    // tick spritesheet animation
    if (boss.spriteSheet)
        boss.spriteSheet->Update(dt);
}
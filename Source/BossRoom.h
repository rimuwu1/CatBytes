/* Start Header ************************************************************************/
/*!
\file BossRoom.h
\author Sim Hui Min, s.huimin, 2503506
        Tse Xuan Qi Tristin, tse.x, 2503757
\par    s.huimin@digipen.edu
        tse.x@digipen.edu
\date Junuary, 24, 2026
\brief
Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "rapidjson/document.h"
#include "SpriteSheet.h"
#include <string>
#include <vector>

// ============================================================
// monitor laser state machine
// ============================================================
enum class MonitorLaserState
{
    Idle,       // LasersIdle (firing during this phase)
    Tracking,   // laser tracks player live
    LockedOn,   // locked-on telegraph before firing
    Firing,     // beam is active and dealing damage
    Cooldown    // brief pause before returning to Idle
};

// ============================================================
// single monitor
// ============================================================
struct Monitor
{
    float        x      = 0.f;
    float        y      = 0.f;
    float        width  = 120.f;
    float        height = 90.f;

    SpriteSheet* sprite = nullptr;   // owns this pointer
    std::string  idleClipName;       // eg. "Monitor3"

    MonitorLaserState laserState    = MonitorLaserState::Idle;
    float             laserTimer    = 0.f;
    float             laserTrackedX = 0.f;  // X locked when transitioning to LockedOn
    float             laserLength   = 0.f;  // current beam length (pixels, downward)
    float laserTrackedY = -325.f; 

    bool isCenter = false;   // true only for index 4 (the center monitor)
};

// ============================================================
// clip definition loaded from JSON
// ============================================================
struct MonitorClipDef
{
    std::string name;
    int         start    = 0;
    int         end      = 0;
    float       duration = 0.1f;
    bool        loop     = true;
};

// ============================================================
// BossRoom singleton
// ============================================================
class BossRoom
{
public:
    static BossRoom& Get() {
        static BossRoom instance;
        return instance;
    }
    void Initialize(const rapidjson::Document& doc);
    void Update(float dt);
    void Draw();
    void Free();
    void Unload();
private:
    BossRoom() = default;
    ~BossRoom() = default;
    BossRoom(const BossRoom&) = delete;
    BossRoom& operator=(const BossRoom&) = delete;
};

// free functions for GSM
void BossRoom_Load();
void BossRoom_Initialize();
void BossRoom_Update();
void BossRoom_Draw();
void BossRoom_Free();
void BossRoom_Unload();
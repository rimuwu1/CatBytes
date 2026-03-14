/* Start Header ************************************************************************/
/*!
\file       Buff.h
\author     Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par        kerwinjiajie.wong@digipen.edu
\date       Mar 10 2026
\brief		This file defines the Buff class and BuffType enum for collectible pickups
            dropped by enemies or placed in levels.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include <memory>

struct Player;

// Buff types
enum class BuffType {
    NONE,
    SHIELD,
    FULL_HP,
    GOD_MODE
};

class Buff {
public:
    BuffType type = BuffType::NONE;
    AEVec2 pos = { 0.0f, 0.0f };
    float width = 50.0f;
    float height = 50.0f;
    bool active = true;
    AEGfxTexture* texture = nullptr;

    //Buff() = default;
    Buff(BuffType type, float x, float y, float width, float height);

    void Activate(Player& player); // activate buff
    void Deactivate(Player& player); // deactivate buff

    void Update(float dt);
    void Draw(float camX, float camY) const;
};

void Player_PickupBuff(Player& player, Buff& buff);
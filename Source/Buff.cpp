/* Start Header ************************************************************************/
/*!
\file       Buff.cpp
\author     Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par        kerwinjiajie.wong@digipen.edu
\date       Mar 10 2026
\brief		This file implements the Buff class. It handles the buff effects on the
            player, drawing the buffs in-game, and pickup behaviour.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "buff.h"
#include "Player.h"
#include "MeshManager.h"

Buff::Buff(BuffType type, float x, float y, float width, float height)
    : type(type), pos({ x, y }), width(width), height(height)
{
    switch (type) {
    case BuffType::SHIELD:
        texture = TextureManager::Get().LoadTexture("Assets/Images/buff_shield.png");
        break;
    case BuffType::FULL_HP:
        texture = TextureManager::Get().LoadTexture("Assets/Images/buff_full_hp.png");
        break;
    case BuffType::DASH:
        texture = TextureManager::Get().LoadTexture("Assets/Images/buff_dash.png");
        break;
    default:
        break;
    }
}

void Buff::Activate(Player& player) {
    switch (type) {
    case BuffType::SHIELD:
        player.shieldActive = true;
        player.shieldTimer = 10.0f;
        break;
    case BuffType::FULL_HP:
        player.hp = player.maxHP;
        break;
    case BuffType::DASH:
        player.dashEnabled = true;
        player.dashCharges++;
        break;
    default:
        break;
    }
}

void Buff::Deactivate(Player& player) {
    active = false;
    switch (type) {
    case BuffType::SHIELD:
        player.shieldActive = false;
        player.shieldTimer = 0.0f;
        break;
    case BuffType::DASH:
        // activate godmode (TBA)
        break;
    default:
        break;
    }
}

void Buff::Update(float dt) {
    // Update logic e.g. buff expiry timer etc (TBA)
    (void)dt; // TBA
}

void Buff::Draw(float camX, float camY) const {
    (void)camX;
    (void)camY;
    if (!active || !texture) return;
    MeshManager::Get().DrawTexturedSquare(texture, pos.x, pos.y, width, height, 1.0f);
}

// Pickup: move buff into player inventory, mark world instance inactive
// Does NOT activate - player uses it from inventory later
void Player_PickupBuff(Player& player, Buff& buff) {
    if (!buff.active) return;
    buff.active = false;                        // Remove from world
    player.buffs.push_back(std::move(buff));    // Store in inventory
    player.buffs.back().active = true;          // Mark as active in inventory (for saving)
}
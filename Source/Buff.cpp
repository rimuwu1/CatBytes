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
#include "AudioManager.h"
#include "ParticleManager.h"

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
        AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("shield_power_up"), false);
        player.shieldActive = true;
        player.shieldTimer = 10.0f;
        break;

    case BuffType::FULL_HP:
        AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("healing_power_up"), false);
        player.hp = player.maxHP;
        // Emit green healing particles
        ParticleManager_Emit(player.pos.x, player.pos.y, 15, 150.0f, 50, 255, 50);
        break;

    case BuffType::DASH:
        AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("dash_power_up"), false);
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
    AudioManager::Get().PlayAudio(AudioManager::Get().GetAudio("buff_collection"), false);
    if (!buff.active) return;
    buff.active = false;                        // Remove from world
    player.buffs.push_back(std::move(buff));    // Store in inventory
    player.buffs.back().active = true;          // Mark as active in inventory (for saving)
}
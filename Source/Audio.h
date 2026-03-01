/* Start Header ************************************************************************/
/*!
\file Audio.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief Centralised list of all audio file paths used in the game

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include <string>

namespace Audio
{
    extern const std::string MAIN_MENU_MUSIC;
    extern const std::string GAME_MUSIC;

    extern const std::string HOVER_BUTTON;
    extern const std::string CLICK_BUTTON;

    extern const std::string JUMP;
    extern const std::string PLAYER_GUN_ATTACK;
    extern const std::string PLAYER_MELEE_ATTACK;

    extern const std::string EASY_ENEMY_ATTACK;
    extern const std::string HARD_ENEMY_ATTACK;
    extern const std::string BOSS_ATTACK;

    extern const std::string WIN_EFFECT;
    extern const std::string LOSE_EFFECT;
}

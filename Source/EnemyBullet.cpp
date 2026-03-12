/* Start Header ************************************************************************/
/*!
\file EnemyBullet.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
        Joash ng, joash.ng, 2502780
\par    tse.x@digipen.edu
        joash.ng@digipen.edu
\date Junuary, 24, 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "EnemyBullet.h"
#include "MeshManager.h"
#include "TextureManager.h"

void EnemyBullet_Draw(const EnemyBullet& bullet)
{
    if (!bullet.active) return;

    // use sprite sheet if available, otherwise fallback to texture
    if (bullet.bulletSprite) {
        MeshManager::Get().DrawSpriteSheet(
            *bullet.bulletSprite,
            bullet.pos.x,
            bullet.pos.y,
            bullet.width,
            bullet.height,
            1.0f    
        );
    }
    else {
        MeshManager::Get().DrawTexturedSquare(
            TextureManager::Get().LoadTexture("Assets/Images/EasyEnemyBullet.jpg"),
            bullet.pos.x,
            bullet.pos.y,
            bullet.width,
            bullet.height,
            1.0f
        );
    }
}

void EnemyBullet_Free()
{
    
}

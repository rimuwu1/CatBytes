/* Start Header ************************************************************************/
/*!
\file EnemyBullet.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
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

//static mesh & texture shared by all bullets
static AEGfxTexture* bulletTexture = nullptr;

void EnemyBullet_Draw(const EnemyBullet& bullet)
{
    if (!bullet.active) return;

    //create mesh if not already(dont create them)
    /*
    if (!bulletMesh)
        bulletMesh = util::CreateSquareMesh();
        */

    //load texture if not already
    if (!bulletTexture)
        bulletTexture = TextureManager::Get().LoadTexture("Assets/Images/EasyEnemyBullet.jpg");

    MeshManager::Get().DrawTexturedSquare(
        bulletTexture,
        bullet.pos.x,
        bullet.pos.y,
        bullet.width,
        bullet.height
    );
}

void EnemyBullet_Free()
{
    if (bulletTexture)
    {
        bulletTexture = nullptr;
    }
    
}

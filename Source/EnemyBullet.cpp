#include "EnemyBullet.h"
#include "Utils.h"

//static mesh & texture shared by all bullets
static AEGfxVertexList* bulletMesh = nullptr;
static AEGfxTexture* bulletTexture = nullptr;

void EnemyBullet_Draw(const EnemyBullet& bullet)
{
    if (!bullet.active) return;

    //create mesh if not already
    if (!bulletMesh)
        bulletMesh = util::CreateSquareMesh();

    //load texture if not already
    if (!bulletTexture)
        bulletTexture = AEGfxTextureLoad("Assets/Images/EasyEnemyBullet.jpg");

    util::DrawTexturedSquare(
        bulletMesh,
        bulletTexture,
        bullet.pos.x,
        bullet.pos.y,
        30.0f,//bullet width
        10.0f,// bullet height
        1.0f// fully opaque
    );
}

void EnemyBullet_Free()
{
    if (bulletTexture)
    {
        AEGfxTextureUnload(bulletTexture);
        bulletTexture = nullptr;
    }

    if (bulletMesh)
    {
        AEGfxMeshFree(bulletMesh);
        bulletMesh = nullptr;
    }
}

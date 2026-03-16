/* Start Header ************************************************************************/
/*!
\file ParticleManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 16/03/2026
\brief This file defines the particle manager class that manages particle randomisation and lifetime.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"
#include "ParticleManager.h"
#include "MeshManager.h"
#include "AEEngine.h"
#include <vector>
#include <cmath>

struct Particle {
    AEVec2 pos, vel;
    float life, maxLife, size;
    int r, g, b;
    bool active = false;
};

static std::vector<Particle> s_particles;

void ParticleManager_Init() {
    s_particles.clear();
    s_particles.resize(256);
}

void ParticleManager_Emit(float x, float y, int count, float speed,
                          int r, int g, int b) {
    int spawned = 0;
    for (auto& p : s_particles) {
        if (p.active) continue;
        float angle = ((float)rand() / RAND_MAX) * 6.2831853f;
        float s     = speed * (0.5f + (float)rand() / RAND_MAX * 0.5f);
        p.pos     = { x, y };
        p.vel     = { cosf(angle) * s, sinf(angle) * s };
        p.life    = p.maxLife = 0.3f + ((float)rand() / RAND_MAX) * 0.2f;
        p.size    = 6.0f + ((float)rand() / RAND_MAX) * 6.0f;
        p.r = r; p.g = g; p.b = b;
        p.active  = true;
        if (++spawned >= count) break;
    }
}

void ParticleManager_Update(float dt) {
    for (auto& p : s_particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.active = false; continue; }
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y -= 300.0f * dt;
    }
}

void ParticleManager_Draw() {
    for (const auto& p : s_particles) {
        if (!p.active) continue;
        float alpha = p.life / p.maxLife;
        MeshManager::Get().DrawSquare(p.pos.x, p.pos.y,
            p.size, p.size, p.r, p.g, p.b, alpha);
    }
}

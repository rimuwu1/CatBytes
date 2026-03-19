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
    AEGfxTexture* texture = nullptr;
    bool active = false;
};

static std::vector<Particle> s_particles;

struct Emitter {
    float x, y;
    float speed;
    float particleTimer = 0.0f;
    float particleInterval = 0.0f; // seconds between spawns
    float minLife, maxLife;
    float minSize, maxSize;
    int r, g, b;
    AEGfxTexture* texture = nullptr; // null = colour mode
    bool active = false;
};

static constexpr int MAX_EMITTERS = 16;
static Emitter s_emitters[MAX_EMITTERS];

void ParticleManager_Init() {
    s_particles.clear();
    s_particles.resize(256);
    for (auto& em : s_emitters) em.active = false;
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
    // Tick emitters
    for (auto& em : s_emitters) {
        if (!em.active) continue;
        em.particleTimer += dt;
        while (em.particleTimer >= em.particleInterval) {
            em.particleTimer -= em.particleInterval;
            // Spawn one particle
            for (auto& p : s_particles) {
                if (p.active) continue;
                float angle = ((float)rand() / RAND_MAX) * 6.2831853f;
                float s = em.speed * (0.5f + (float)rand() / RAND_MAX * 0.5f);
                p.pos     = { em.x, em.y };
                p.vel     = { cosf(angle) * s, sinf(angle) * s };
                float lifeRange = em.maxLife - em.minLife;
                p.life    = p.maxLife = em.minLife + ((float)rand() / RAND_MAX) * lifeRange;
                float sizeRange = em.maxSize - em.minSize;
                p.size    = em.minSize + ((float)rand() / RAND_MAX) * sizeRange;
                p.r = em.r; p.g = em.g; p.b = em.b;
                p.texture = em.texture;
                p.active  = true;
                break;
            }
        }
    }

    for (auto& p : s_particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.active = false; continue; }
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y -= 300.0f * dt;
    }
}

void ParticleManager_EmitTextured(float x, float y, int count, float speed,
                                  AEGfxTexture* tex,
                                  float minLife, float maxLife,
                                  float minSize, float maxSize)
{
    int spawned = 0;
    for (auto& p : s_particles) {
        if (p.active) continue;
        float angle = ((float)rand() / RAND_MAX) * 6.2831853f;
        float s     = speed * (0.5f + (float)rand() / RAND_MAX * 0.5f);
        p.pos     = { x, y };
        p.vel     = { cosf(angle) * s, sinf(angle) * s };
        float lr  = maxLife - minLife;
        p.life    = p.maxLife = minLife + ((float)rand() / RAND_MAX) * lr;
        float sr  = maxSize - minSize;
        p.size    = minSize + ((float)rand() / RAND_MAX) * sr;
        p.texture = tex;
        p.r = p.g = p.b = 255;
        p.active  = true;
        if (++spawned >= count) break;
    }
}

EmitterHandle ParticleManager_EmitterStart(
    float x, float y, int particlesPerSecond, float speed,
    int r, int g, int b, float minLife, float maxLife,
    float minSize, float maxSize)
{
    for (int i = 0; i < MAX_EMITTERS; ++i) {
        if (s_emitters[i].active) continue;
        s_emitters[i].x = x;
        s_emitters[i].y = y;
        s_emitters[i].speed = speed;
        s_emitters[i].particleTimer = 0.0f;
        s_emitters[i].particleInterval = particlesPerSecond > 0 ? 1.0f / particlesPerSecond : 1.0f;
        s_emitters[i].minLife = minLife; s_emitters[i].maxLife = maxLife;
        s_emitters[i].minSize = minSize; s_emitters[i].maxSize = maxSize;
        s_emitters[i].r = r; s_emitters[i].g = g; s_emitters[i].b = b;
        s_emitters[i].texture = nullptr;
        s_emitters[i].active = true;
        return i;
    }
    return INVALID_EMITTER; // pool exhausted
}

EmitterHandle ParticleManager_EmitterStartTextured(
    float x, float y, int particlesPerSecond, float speed,
    AEGfxTexture* tex, float minLife, float maxLife,
    float minSize, float maxSize)
{
    for (int i = 0; i < MAX_EMITTERS; ++i) {
        if (s_emitters[i].active) continue;
        s_emitters[i].x = x;
        s_emitters[i].y = y;
        s_emitters[i].speed = speed;
        s_emitters[i].particleTimer = 0.0f;
        s_emitters[i].particleInterval = particlesPerSecond > 0 ? 1.0f / particlesPerSecond : 1.0f;
        s_emitters[i].minLife = minLife; s_emitters[i].maxLife = maxLife;
        s_emitters[i].minSize = minSize; s_emitters[i].maxSize = maxSize;
        s_emitters[i].r = 255; s_emitters[i].g = 255; s_emitters[i].b = 255;
        s_emitters[i].texture = tex;
        s_emitters[i].active = true;
        return i;
    }
    return INVALID_EMITTER;
}

void ParticleManager_EmitterMove(EmitterHandle handle, float x, float y)
{
    if (handle < 0 || handle >= MAX_EMITTERS) return;
    if (!s_emitters[handle].active) return;
    s_emitters[handle].x = x;
    s_emitters[handle].y = y;
}

void ParticleManager_EmitterStop(EmitterHandle handle)
{
    if (handle < 0 || handle >= MAX_EMITTERS) return;
    s_emitters[handle].active = false;
}

void ParticleManager_Draw() {
    for (const auto& p : s_particles) {
        if (!p.active) continue;
        float alpha = p.life / p.maxLife;
        if (p.texture) {
            MeshManager::Get().DrawTexturedSquare(p.texture, p.pos.x, p.pos.y, p.size, p.size, alpha);
        } else {
            MeshManager::Get().DrawSquare(p.pos.x, p.pos.y,
                p.size, p.size, p.r, p.g, p.b, alpha);
        }
    }
}

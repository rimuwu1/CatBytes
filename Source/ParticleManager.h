/* Start Header ************************************************************************/
/*!
\file ParticleManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief This file defines the particle manager class that manages particle randomisation and lifetime.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "AEEngine.h"

void ParticleManager_Init();
void ParticleManager_Emit(float x, float y, int count, float speed,
                          int r, int g, int b);
// Emit dust particles in an upward hemisphere (for jump/land effects)
void ParticleManager_EmitDust(float x, float y, int count, float speed,
                              int r, int g, int b);
// Textured one-shot burst — pass a pre-loaded AEGfxTexture*
void ParticleManager_EmitTextured(float x, float y, int count, float speed,
                                  AEGfxTexture* tex,
                                  float minLife = 0.3f, float maxLife = 0.5f,
                                  float minSize = 6.0f, float maxSize = 12.0f);

void ParticleManager_Update(float dt);
void ParticleManager_Draw();

// Emitter handle — store this to control a running emitter
using EmitterHandle = int;
static constexpr EmitterHandle INVALID_EMITTER = -1;

// Start a continuous emitter. Returns a handle to control it.
EmitterHandle ParticleManager_EmitterStart(
    float x, float y,
    int   particlesPerSecond,
    float speed,
    int r, int g, int b,
    float minLife = 0.2f, float maxLife = 0.4f,
    float minSize = 4.0f, float maxSize = 8.0f);

// Textured continuous emitter
EmitterHandle ParticleManager_EmitterStartTextured(
    float x, float y,
    int   particlesPerSecond,
    float speed,
    AEGfxTexture* tex,
    float minLife = 0.2f, float maxLife = 0.4f,
    float minSize = 4.0f, float maxSize = 8.0f);

// Update emitter position (call every frame for moving emitters)
void ParticleManager_EmitterMove(EmitterHandle handle, float x, float y);

// Stop a running emitter
void ParticleManager_EmitterStop(EmitterHandle handle);

// Tick all active emitters — called inside ParticleManager_Update

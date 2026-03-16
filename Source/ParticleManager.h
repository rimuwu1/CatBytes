#pragma once

void ParticleManager_Init();
void ParticleManager_Emit(float x, float y, int count, float speed,
                          int r, int g, int b);
void ParticleManager_Update(float dt);
void ParticleManager_Draw();

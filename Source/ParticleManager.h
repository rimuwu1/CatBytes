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

void ParticleManager_Init();
void ParticleManager_Emit(float x, float y, int count, float speed,
                          int r, int g, int b);
void ParticleManager_Update(float dt);
void ParticleManager_Draw();

/* Start Header ************************************************************************/
/*!
\file       LevelIndicator.h
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 25 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "AEEngine.h"

void LevelIndicator_Initialize();

void LevelIndicator_Show(int sectionIndex);

void LevelIndicator_Update(float dt);

void LevelIndicator_Draw();
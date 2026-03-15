/* Start Header ************************************************************************/
/*!
\file BossRoom.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par    tse.x@digipen.edu
\date Junuary, 24, 2026
\brief

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

class BossRoom
{
public:
    static void Load();
    static void Initialize();
    static void Update(float dt);
    static void Draw();
    static void Free();
    static void Unload();
};
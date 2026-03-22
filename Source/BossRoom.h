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
#include "rapidjson/document.h"

class BossRoom
{
public:
    static BossRoom& Get() {
        static BossRoom instance;
        return instance;
    }
    void Initialize(const rapidjson::Document& doc);
    void Update(float dt);
    void Draw();
    void Free();
    void Unload();
private:
    BossRoom() = default;
    ~BossRoom() = default;
    BossRoom(const BossRoom&) = delete;
    BossRoom& operator=(const BossRoom&) = delete;
};

// Free functions for GSM
void BossRoom_Load();
void BossRoom_Initialize();
void BossRoom_Update();
void BossRoom_Draw();
void BossRoom_Free();
void BossRoom_Unload();
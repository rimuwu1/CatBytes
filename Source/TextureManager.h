/* Start Header ************************************************************************/
/*!
\file TextureManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief This file declares util functions to dynamically manage textures.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include <map>
#include <string>

class TextureManager {
public:
    static TextureManager& Get() {
        static TextureManager instance;
        return instance;
    }

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // Load texture if not already loaded; returns pointer (cached)
    AEGfxTexture* LoadTexture(const std::string& filepath);

    // manual unload (don't need to use unles really need to unload specific texture)
    void UnloadTexture(const std::string& filepath);

    // Unload all textures (called automatically in destructor)
    void UnloadAll();

private:
    TextureManager() = default;
    ~TextureManager();  // Unloads all textures

    std::map<std::string, AEGfxTexture*> textureMap;
};
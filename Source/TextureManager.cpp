/* Start Header ************************************************************************/
/*!
\file TextureManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief This file implements util functions to dynamically manage textures.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "TextureManager.h"

TextureManager::~TextureManager() {
    UnloadAll();
}

AEGfxTexture* TextureManager::LoadTexture(const std::string& filepath) {
    auto it = textureMap.find(filepath);
    if (it != textureMap.end())
        return it->second;

    AEGfxTexture* tex = AEGfxTextureLoad(filepath.c_str());
    if (tex)
        textureMap[filepath] = tex;
    return tex;
}
//only if NEED to manually unload tex
void TextureManager::UnloadTexture(const std::string& filepath) {
    auto it = textureMap.find(filepath);
    if (it != textureMap.end()) {
        AEGfxTextureUnload(it->second);
        textureMap.erase(it);
    }
}

void TextureManager::UnloadAll() {
    for (auto const& pair : textureMap) {
        if (pair.second) AEGfxTextureUnload(pair.second);
    }
    textureMap.clear();
}

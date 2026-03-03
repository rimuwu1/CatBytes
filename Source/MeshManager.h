/* Start Header ************************************************************************/
/*!
\file MeshManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief This file declares the meshmanager class that manages dynamically allocated meshes.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "AEEngine.h"
#include "SpriteSheet.h"
#include <map>
#include <string>

class MeshManager {
public:
    static MeshManager& Get() {
        static MeshManager instance;
        return instance;
    }

    MeshManager(const MeshManager&) = delete;
    MeshManager& operator=(const MeshManager&) = delete;

    AEGfxVertexList* GetMesh(const std::string& name);

    // ----- Colour drawing helpers with defaults -----

    void DrawSquare(float x, float y, float width, float height,
        int r = 255, int g = 0, int b = 0, float opacity = 1.0f);          // default red

    void DrawCircle(float x, float y, float diameter,
        int r = 255, int g = 0, int b = 0, float opacity = 1.0f);          // default red

    void DrawTriangle(float x, float y, float width, float height,
        float rotationDeg = 0.0f,
        int r = 128, int g = 128, int b = 128, float opacity = 1.0f);    // default gray

    void DrawLine(float x1, float y1, float x2, float y2,
        float thickness, int r = 255, int g = 0, int b = 0, //default red
        float opacity = 1.0f);
   

    // ----- Textured drawing helpers with defaults -----

    void DrawTexturedSquare(AEGfxTexture* texture,
        float x, float y, float width, float height,
        float opacity = 1.0f);

    void DrawSpriteSheet(SpriteSheet& sprite,
        float x, float y, float width, float height,
        float opacity = 1.f, float rotationDeg = 0.f);

    void DrawTexturedLine(AEGfxTexture* texture,
        float x1, float y1, float x2, float y2,
        float thickness,
        float tileLength,
        float opacity = 1.0f);

    //cleanup functions
    void UnloadAll();
private:
    MeshManager() = default;
    ~MeshManager();

    std::map<std::string, AEGfxVertexList*> meshMap;

    AEGfxVertexList* CreateSquareMesh();
    AEGfxVertexList* CreateCircleMesh();
    AEGfxVertexList* CreateTriangleMesh();
    AEGfxVertexList* CreateLineMesh();
};
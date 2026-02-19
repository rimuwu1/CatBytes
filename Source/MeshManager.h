#pragma once
#include "AEEngine.h"
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
        int r = 255, int g = 0, int b = 0);          // default red

    void DrawCircle(float x, float y, float diameter,
        int r = 255, int g = 0, int b = 0);          // default red

    void DrawTriangle(float x, float y, float width, float height,
        float rotationDeg = 0.0f,
        int r = 128, int g = 128, int b = 128);    // default gray

    // ----- Textured drawing helpers with defaults -----
    void DrawTexturedSquare(AEGfxTexture* texture,
        float x, float y, float width, float height,
        float opacity = 1.0f);

    void DrawTexturedSquarePivot(AEGfxTexture* texture,
        float x, float y, float width, float height,
        float rotationDeg, float pivotX, float pivotY,
        float opacity = 1.0f);

private:
    MeshManager() = default;
    ~MeshManager();

    std::map<std::string, AEGfxVertexList*> meshMap;

    AEGfxVertexList* CreateSquareMesh();
    AEGfxVertexList* CreateCircleMesh();
    AEGfxVertexList* CreateTriangleMesh();
};
/* Start Header ************************************************************************/
/*!
\file MeshManager.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief This file implements util functions to create mesh and draw mesh.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "MeshManager.h"
#include <cmath>

MeshManager::~MeshManager() {
    for (auto const& pair : meshMap) {
        if (pair.second) AEGfxMeshFree(pair.second);
    }
    meshMap.clear();
}

AEGfxVertexList* MeshManager::GetMesh(const std::string& name) {
    auto it = meshMap.find(name);
    if (it != meshMap.end()) return it->second;

    AEGfxVertexList* newMesh = nullptr;
    if (name == "square")   newMesh = CreateSquareMesh();
    else if (name == "circle")  newMesh = CreateCircleMesh();
    else if (name == "triangle") newMesh = CreateTriangleMesh();

    if (newMesh) meshMap[name] = newMesh;
    return newMesh;
}

// --- Mesh creation ---
AEGfxVertexList* MeshManager::CreateSquareMesh() {
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

AEGfxVertexList* MeshManager::CreateCircleMesh() {
    AEGfxMeshStart();
    const int segments = 42;
    float radius = 0.5f;
    float angleStep = 2.0f * 3.14159265f / segments;
    for (int i = 0; i < segments; ++i) {
        float a1 = i * angleStep;
        float a2 = (i + 1) * angleStep;
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,
            cosf(a1) * radius, sinf(a1) * radius, 0xFFFFFFFF, 0.0f, 0.0f,
            cosf(a2) * radius, sinf(a2) * radius, 0xFFFFFFFF, 0.0f, 0.0f);
    }
    return AEGfxMeshEnd();
}

AEGfxVertexList* MeshManager::CreateTriangleMesh() {
    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFFFFFF, 0.5f, 0.0f,
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    return AEGfxMeshEnd();
}

// --- Drawing implementations ---
void MeshManager::DrawSquare(float x, float y, float width, float height,
    int r, int g, int b) {
    AEGfxVertexList* mesh = GetMesh("square");
    AEMtx33 scale, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawCircle(float x, float y, float diameter,
    int r, int g, int b) {
    AEGfxVertexList* mesh = GetMesh("circle");
    AEMtx33 scale, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33Scale(&scale, diameter, diameter);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawTriangle(float x, float y, float width, float height,
    float rotationDeg, int r, int g, int b) {
    AEGfxVertexList* mesh = GetMesh("triangle");
    AEMtx33 scale, rotate, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33RotDeg(&rotate, rotationDeg);
    AEMtx33Trans(&translate, x, y);

    // transform = translate * rotate * scale
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawTexturedSquare(AEGfxTexture* texture,
    float x, float y, float width, float height,
    float opacity) {
    AEGfxVertexList* mesh = GetMesh("square");
    AEMtx33 scale, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawSpriteSheet(SpriteSheet& sprite,
    float x, float y, float width, float height,
    float opacity)
{
    float uvW = sprite.GetSpriteUVWidth();
    float uvH = sprite.GetSpriteUVHeight();

    // Build a fresh mesh sized to one sprite cell's UV dimensions
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, uvH,
        0.5f, -0.5f, 0xFFFFFFFF, uvW, uvH,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, uvW, uvH,
        0.5f, 0.5f, 0xFFFFFFFF, uvW, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexList* mesh = AEGfxMeshEnd();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);
    AEGfxTextureSet(sprite.GetTexture(), sprite.GetUVOffsetX(), sprite.GetUVOffsetY());

    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);

    AEGfxMeshFree(mesh);
}

//delete when not needed (after removing the sword code)
void MeshManager::DrawTexturedSquarePivot(AEGfxTexture* texture,
    float x, float y, float width, float height,
    float rotationDeg, float pivotX, float pivotY,
    float opacity) {
    AEGfxVertexList* mesh = GetMesh("square");
    AEMtx33 scale, rot, trans, pivot, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&pivot, -pivotX, -pivotY);
    AEMtx33RotDeg(&rot, rotationDeg);
    AEMtx33Trans(&trans, x, y);

    // T * R * P * S
    AEMtx33Concat(&transform, &pivot, &scale);
    AEMtx33Concat(&transform, &rot, &transform);
    AEMtx33Concat(&transform, &trans, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}
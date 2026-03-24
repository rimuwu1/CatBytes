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
#include <sstream>

MeshManager::~MeshManager() {
    if (!meshMap.empty())
        UnloadAll();
}

static size_t HashBatchKey(void* texture, float uvW, float uvH) {
    std::hash<std::uintptr_t> hasher;
    size_t h1 = hasher(reinterpret_cast<std::uintptr_t>(texture));
    std::hash<float> hasherF;
    size_t h2 = hasherF(uvW);
    size_t h3 = hasherF(uvH);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

void MeshManager::BeginBatch(AEGfxTexture* texture, float uvW, float uvH) {
    size_t newKey = HashBatchKey(texture, uvW, uvH);

    // Auto-flush previous batch if different texture+UV
    if (m_batchActive && m_currentBatchKey != newKey) {
        EndBatch();
    }

    m_currentBatchKey = newKey;
    auto& batch = m_batches[m_currentBatchKey];
    batch.texture = texture;
    batch.uvW = uvW;
    batch.uvH = uvH;
    // Don't clear items - accumulate!
    m_batchActive = true;
}

void MeshManager::QueueSprite(const SpriteBatchItem& sprite) {
    if (!m_batchActive) return;
    auto it = m_batches.find(m_currentBatchKey);
    if (it != m_batches.end()) {
        it->second.items.push_back(sprite);
    }
}

void MeshManager::EndBatch() {
    if (!m_batchActive) return;
    m_batchActive = false;

    auto it = m_batches.find(m_currentBatchKey);
    if (it == m_batches.end() || it->second.items.empty()) return;

    Batch& batch = it->second;
    AEGfxVertexList* mesh = GetOrCreateSpriteMesh(batch.uvW, batch.uvH);
    if (!mesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxTextureSet(batch.texture, 0, 0);

    //const float degToRad = 3.1415926535f / 180.0f;

    for (const auto& sprite : batch.items) {
        AEGfxSetTransparency(sprite.opacity);

        //float rotRad = sprite.rotation * degToRad;
        AEMtx33 scale, rotate, translate, transform;
        AEMtx33Scale(&scale, sprite.width, sprite.height);
        AEMtx33RotDeg(&rotate, sprite.rotation);
        AEMtx33Trans(&translate, sprite.x, sprite.y);

        AEMtx33 tmp;
        AEMtx33Concat(&tmp, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &tmp);

        AEGfxTextureSet(batch.texture, sprite.uvOffsetX, sprite.uvOffsetY);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }

    batch.items.clear();
}

AEGfxVertexList* MeshManager::GetMesh(const std::string& name) {
    auto it = meshMap.find(name);
    if (it != meshMap.end()) return it->second;

    AEGfxVertexList* newMesh = nullptr;
    if (name == "square")   newMesh = CreateSquareMesh();
    else if (name == "circle")  newMesh = CreateCircleMesh();
    else if (name == "triangle") newMesh = CreateTriangleMesh();
    else if (name == "line")     newMesh = CreateLineMesh();

    if (newMesh) meshMap[name] = newMesh;
    return newMesh;
}

AEGfxVertexList* MeshManager::GetOrCreateSpriteMesh(float uvW, float uvH) {
    std::ostringstream key;
    key << "sprite_" << uvW << "_" << uvH;
    std::string keyStr = key.str();

    auto it = meshMap.find(keyStr);
    if (it != meshMap.end()) return it->second;

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, uvH,
        0.5f, -0.5f, 0xFFFFFFFF, uvW, uvH,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, uvW, uvH,
        0.5f, 0.5f, 0xFFFFFFFF, uvW, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexList* mesh = AEGfxMeshEnd();

    meshMap[keyStr] = mesh;
    return mesh;
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

AEGfxVertexList* MeshManager::CreateLineMesh() {
    AEGfxMeshStart();
    // First triangle
    AEGfxTriAdd(0.0f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        1.0f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        0.0f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    // Second triangle
    AEGfxTriAdd(1.0f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        1.0f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.0f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    return AEGfxMeshEnd();
}

// --- Drawing implementations ---
void MeshManager::DrawSquare(float x, float y, float width, float height,
    int r, int g, int b, float opacity) {
    AEGfxVertexList* mesh = GetMesh("square");
    AEMtx33 scale, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(opacity);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawCircle(float x, float y, float diameter,
    int r, int g, int b, float opacity) {
    AEGfxVertexList* mesh = GetMesh("circle");
    AEMtx33 scale, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(opacity);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33Scale(&scale, diameter, diameter);
    AEMtx33Trans(&translate, x, y);
    AEMtx33Concat(&transform, &translate, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawTriangle(float x, float y, float width, float height,
    float rotationDeg, int r, int g, int b, float opacity) {
    AEGfxVertexList* mesh = GetMesh("triangle");
    AEMtx33 scale, rotate, translate, transform;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(opacity);
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
    float x, float y,
    float width, float height,
    float opacity,
    float rotationDeg)
{
    float uvW = sprite.GetSpriteUVWidth();
    float uvH = sprite.GetSpriteUVHeight();

    AEGfxVertexList* mesh = GetOrCreateSpriteMesh(uvW, uvH);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);
    AEGfxTextureSet(sprite.GetTexture(), sprite.GetUVOffsetX(), sprite.GetUVOffsetY());

    // Build transformation matrix: Scale -> Rotate -> Translate
    const float degToRad = 3.1415926535f / 180.0f;
    float rotRad = rotationDeg * degToRad;

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Rot(&rotate, rotRad);                 // rotation around origin (center of sprite)
    AEMtx33Trans(&translate, x, y);

    // Combine: transform = translate * rotate * scale
    AEMtx33 tmp;
    AEMtx33Concat(&tmp, &rotate, &scale);        // tmp = rotate * scale
    AEMtx33Concat(&transform, &translate, &tmp); // transform = translate * tmp

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawLine(float x1, float y1, float x2, float y2,
    float thickness, int r, int g, int b,
    float opacity) {
    AEGfxVertexList* mesh = GetMesh("line");
    if (!mesh) return;

    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 1e-6f) return;  // nothing to draw (epsilon)

    float angle = atan2f(dy, dx);  // angle from X axis to direction

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(r / 255.0f, g / 255.0f, b / 255.0f, opacity);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, length, thickness);
    AEMtx33Rot(&rotate, angle);
    AEMtx33Trans(&translate, x1, y1);

    AEMtx33 tmp;
    AEMtx33Concat(&tmp, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &tmp);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void MeshManager::DrawTexturedLine(AEGfxTexture* texture,
    float x1, float y1, float x2, float y2,
    float thickness,
    float tileLength,
    float opacity,
    int tintR, int tintG, int tintB) {
    if (!texture || tileLength <= 0.0f) return;

    float dx = x2 - x1;
    float dy = y2 - y1;
    float totalLength = sqrtf(dx * dx + dy * dy);
    if (totalLength < 1e-6f) return;

    // Direction unit vector
    float dirX = dx / totalLength;
    float dirY = dy / totalLength;
    float angle = atan2f(dy, dx);   // rotation angle for each segment

    // Number of full tiles and the remainder
    int fullTiles = (int)(totalLength / tileLength);
    float remainder = totalLength - fullTiles * tileLength;

    // Set common rendering states and apply tint
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(tintR / 255.0f, tintG / 255.0f, tintB / 255.0f, 1.0f);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(opacity);

    // Pre-allocate transformation matrices
    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Rot(&rotate, angle);          // same rotation for all segments

    // Draw full tiles
    for (int i = 0; i < fullTiles; ++i) {
        // Start point of this segment
        float segStartX = x1 + i * tileLength * dirX;
        float segStartY = y1 + i * tileLength * dirY;

        // Build transform for this segment
        AEMtx33Scale(&scale, tileLength, thickness);
        AEMtx33Trans(&translate, segStartX, segStartY);

        AEMtx33Concat(&transform, &rotate, &scale);      // rotate * scale
        AEMtx33Concat(&transform, &translate, &transform); // translate * (rotate * scale)

        // Use the cached unit line mesh (UVs 0..1 cover the whole tile)
        AEGfxSetTransform(transform.m);
        AEGfxTextureSet(texture, 0, 0);
        AEGfxMeshDraw(GetMesh("line"), AE_GFX_MDM_TRIANGLES);
    }

    // Draw the last partial tile if needed
    if (remainder > 0.0f) {
        float segStartX = x1 + fullTiles * tileLength * dirX;
        float segStartY = y1 + fullTiles * tileLength * dirY;

        AEMtx33Scale(&scale, remainder, thickness);
        AEMtx33Trans(&translate, segStartX, segStartY);
        AEMtx33Concat(&transform, &rotate, &scale);
        AEMtx33Concat(&transform, &translate, &transform);

        AEGfxSetTransform(transform.m);
        AEGfxTextureSet(texture, 0, 0);
        // Reuse cached line mesh — texture will stretch/clamp on partial tile
        // which is visually acceptable for a laser beam
        AEGfxMeshDraw(GetMesh("line"), AE_GFX_MDM_TRIANGLES);
    }
}

void MeshManager::UnloadAll() {
    for (auto const& pair : meshMap) {
        if (pair.second) AEGfxMeshFree(pair.second);
    }
    meshMap.clear();
}

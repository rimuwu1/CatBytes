/* Start Header ************************************************************************/
/*!
\file SpatialGrid.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 08/03/2026
\brief This file implements the SpatialGrid class for 1D Y-axis spatial partitioning.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "pch.h"
#include "SpatialGrid.h"
#include "DebugManager.h"
#include "MeshManager.h"
#include <cmath>

void SpatialGrid::RebuildAdd(
    const std::vector<Platform>& platforms,
    const std::vector<PlatformObstacle>& obstacles
) {
    for (const auto& pf : platforms) {
        if (!pf.active) continue;
        float objMinY = pf.y - pf.h * 0.5f;
        float objMaxY = pf.y + pf.h * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                platformCells[i].push_back(&pf);
            }
        }
    }

    for (const auto& obs : obstacles) {
        float objMinY = obs.y - obs.h * 0.5f;
        float objMaxY = obs.y + obs.h * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                obstacleCells[i].push_back(&obs);
            }
        }
    }
}

void SpatialGrid::Rebuild(
    const std::vector<Platform>& platforms,
    const std::vector<PlatformObstacle>& obstacles,
    const std::vector<Checkpoint>& checkpoints,
    const std::vector<Enemy*>& enemies,
    const std::vector<EnemyBullet*>& bullets
) {
    Clear();

    for (const auto& pf : platforms) {
        if (!pf.active) continue;
        float objMinY = pf.y - pf.h * 0.5f;
        float objMaxY = pf.y + pf.h * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                platformCells[i].push_back(&pf);
            }
        }
    }

    for (const auto& obs : obstacles) {
        float objMinY = obs.y - obs.h * 0.5f;
        float objMaxY = obs.y + obs.h * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                obstacleCells[i].push_back(&obs);
            }
        }
    }

    for (const auto& cp : checkpoints) {
        float objMinY = cp.y - cp.h * 0.5f;
        float objMaxY = cp.y + cp.h * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                checkpointCells[i].push_back(&cp);
            }
        }
    }

    for (auto* enemy : enemies) {
        if (!enemy || !enemy->isAlive) continue;
        float objMinY = enemy->pos.y - enemy->height * 0.5f;
        float objMaxY = enemy->pos.y + enemy->height * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                enemyCells[i].push_back(enemy);
            }
        }
    }

    for (auto* bullet : bullets) {
        if (!bullet || !bullet->active) continue;
        float objMinY = bullet->pos.y - 20.0f * 0.5f;
        float objMaxY = bullet->pos.y + 20.0f * 0.5f;
        int startCell = GetCellIndex(objMinY);
        int endCell = GetCellIndex(objMaxY);
        for (int i = startCell; i <= endCell; ++i) {
            if (i >= 0 && i < numCells) {
                bulletCells[i].push_back(bullet);
            }
        }
    }
}

void SpatialGrid::GetNearbyPlatforms(float y, float height, std::vector<const Platform*>& out) const {
    if (numCells <= 0) return;
    float objMinY = y - height * 0.5f;
    float objMaxY = y + height * 0.5f;
    int startCell = GetCellIndex(objMinY);
    int endCell = GetCellIndex(objMaxY);
    for (int i = startCell; i <= endCell; ++i) {
        if (i >= 0 && i < numCells) {
            for (const auto& obj : platformCells[i]) {
                out.push_back(obj);
            }
        }
    }
}

void SpatialGrid::GetNearbyObstacles(float y, float height, std::vector<const PlatformObstacle*>& out) const {
    if (numCells <= 0) return;
    float objMinY = y - height * 0.5f;
    float objMaxY = y + height * 0.5f;
    int startCell = GetCellIndex(objMinY);
    int endCell = GetCellIndex(objMaxY);
    for (int i = startCell; i <= endCell; ++i) {
        if (i >= 0 && i < numCells) {
            for (const auto& obj : obstacleCells[i]) {
                out.push_back(obj);
            }
        }
    }
}

void SpatialGrid::GetNearbyCheckpoints(float y, float height, std::vector<const Checkpoint*>& out) const {
    if (numCells <= 0) return;
    float objMinY = y - height * 0.5f;
    float objMaxY = y + height * 0.5f;
    int startCell = GetCellIndex(objMinY);
    int endCell = GetCellIndex(objMaxY);
    for (int i = startCell; i <= endCell; ++i) {
        if (i >= 0 && i < numCells) {
            for (const auto& obj : checkpointCells[i]) {
                out.push_back(obj);
            }
        }
    }
}

void SpatialGrid::GetNearbyEnemies(float y, float height, std::vector<Enemy*>& out) const {
    if (numCells <= 0) return;
    float objMinY = y - height * 0.5f;
    float objMaxY = y + height * 0.5f;
    int startCell = GetCellIndex(objMinY);
    int endCell = GetCellIndex(objMaxY);
    for (int i = startCell; i <= endCell; ++i) {
        if (i >= 0 && i < numCells) {
            for (const auto& obj : enemyCells[i]) {
                out.push_back(obj);
            }
        }
    }
}

void SpatialGrid::GetNearbyBullets(float y, float height, std::vector<EnemyBullet*>& out) const {
    if (numCells <= 0) return;
    float objMinY = y - height * 0.5f;
    float objMaxY = y + height * 0.5f;
    int startCell = GetCellIndex(objMinY);
    int endCell = GetCellIndex(objMaxY);
    for (int i = startCell; i <= endCell; ++i) {
        if (i >= 0 && i < numCells) {
            for (const auto& obj : bulletCells[i]) {
                out.push_back(obj);
            }
        }
    }
}

//debug functions
#ifdef ENABLE_DEBUG_MANAGER

void SpatialGrid::DebugPrintCellCounts(DebugManager& dbg) const {
    char buf[96];
    for (int i = 0; i < numCells; ++i) {
        snprintf(buf, sizeof(buf), "Cell[%d]: P=%zu O=%zu C=%zu E=%zu B=%zu",
            i,
            platformCells[i].size(),
            obstacleCells[i].size(),
            checkpointCells[i].size(),
            enemyCells[i].size(),
            bulletCells[i].size());
        dbg.Log(buf);
    }
}

void SpatialGrid::DebugPrintNearby(float y, float height, DebugManager& dbg) const {
    std::vector<const Platform*> platforms;
    std::vector<const PlatformObstacle*> obstacles;
    std::vector<const Checkpoint*> checkpoints;
    std::vector<Enemy*> enemies;
    std::vector<EnemyBullet*> bullets;

    GetNearbyPlatforms(y, height, platforms);
    GetNearbyObstacles(y, height, obstacles);
    GetNearbyCheckpoints(y, height, checkpoints);
    GetNearbyEnemies(y, height, enemies);
    GetNearbyBullets(y, height, bullets);

    char buf[128];
    snprintf(buf, sizeof(buf), "Near Y=%.1f (h=%.1f):", (double)y, (double)height);
    dbg.Log(buf);
    snprintf(buf, sizeof(buf), "  Platforms: %zu", platforms.size());
    dbg.Log(buf);
    snprintf(buf, sizeof(buf), "  Obstacles: %zu", obstacles.size());
    dbg.Log(buf);
    snprintf(buf, sizeof(buf), "  Checkpoints: %zu", checkpoints.size());
    dbg.Log(buf);
    snprintf(buf, sizeof(buf), "  Enemies: %zu", enemies.size());
    dbg.Log(buf);
    snprintf(buf, sizeof(buf), "  Bullets: %zu", bullets.size());
    dbg.Log(buf);
}

void SpatialGrid::DebugDrawGrid() const {
    const float x1 = -1000.0f;
    const float x2 = 1000.0f;

    for (int i = 0; i < numCells; ++i) {
        float cellTop = minWorldY + (i + 1) * cellHeight;
        //float cellBottom = minWorldY + i * cellHeight;

        size_t totalObjects = platformCells[i].size() + obstacleCells[i].size() + 
                              checkpointCells[i].size() + enemyCells[i].size() + 
                              bulletCells[i].size();

        int r = 100, g = 100, b = 100;
        if (totalObjects > 0) {
            r = 0; g = 255; b = 0;
        }

        MeshManager::Get().DrawLine(x1, cellTop, x2, cellTop, 2.0f, r, g, b, 0.7f);
    }

    MeshManager::Get().DrawLine(x1, minWorldY, x2, minWorldY, 3.0f, 255, 255, 255, 0.8f);
    MeshManager::Get().DrawLine(x1, maxWorldY, x2, maxWorldY, 3.0f, 255, 255, 255, 0.8f);
}
#endif

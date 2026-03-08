/* Start Header ************************************************************************/
/*!
\file SpatialGrid.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 08/03/2026
\brief This file declares the SpatialGrid class for 1D Y-axis spatial partitioning.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "Platforms.h"
#include "enemy.h"
#include "EnemyBullet.h"
#include "AEEngine.h"
#include <vector>

class DebugManager;
//DEBUG FUNCTIONS:
//showgrid - draws the grid out, green = has objects, grey = no obj
//grid - Show spatial grid info (bounds, cell size, cell count).
//gridcells - Print object counts in each spatial grid cell
//gridnear/ gridnear[y] - Show nearby objects for Y position

class SpatialGrid {
public:
    SpatialGrid();
    explicit SpatialGrid(float cellHeight, float minY, float maxY);

    void SetWorldBounds(float minY, float maxY);
    void SetCellHeight(float height);

    void Clear();
    void RebuildAdd(
        const std::vector<Platform>& platforms,
        const std::vector<PlatformObstacle>& obstacles
    );
    void Rebuild(
        const std::vector<Platform>& platforms,
        const std::vector<PlatformObstacle>& obstacles,
        const std::vector<Checkpoint>& checkpoints,
        const std::vector<Enemy*>& enemies,
        const std::vector<EnemyBullet*>& bullets
    );

    void GetNearbyPlatforms(float y, float height, std::vector<const Platform*>& out) const;
    void GetNearbyObstacles(float y, float height, std::vector<const PlatformObstacle*>& out) const;
    void GetNearbyCheckpoints(float y, float height, std::vector<const Checkpoint*>& out) const;
    void GetNearbyEnemies(float y, float height, std::vector<Enemy*>& out) const;
    void GetNearbyBullets(float y, float height, std::vector<EnemyBullet*>& out) const;

    int GetCellCount() const { return static_cast<int>(platformCells.size()); }
    float GetCellHeight() const { return cellHeight; }
    float GetMinY() const { return minWorldY; }
    float GetMaxY() const { return maxWorldY; }

    //only for debug
    void DebugPrintCellCounts(DebugManager& dbg) const;
    void DebugPrintNearby(float y, float height, DebugManager& dbg) const;
    void DebugDrawGrid() const;


private:
    int GetCellIndex(float y) const;
    void EnsureCellCount();

    float cellHeight;
    float minWorldY;
    float maxWorldY;
    int numCells;

    std::vector<std::vector<const Platform*>> platformCells;
    std::vector<std::vector<const PlatformObstacle*>> obstacleCells;
    std::vector<std::vector<const Checkpoint*>> checkpointCells;
    std::vector<std::vector<Enemy*>> enemyCells;
    std::vector<std::vector<EnemyBullet*>> bulletCells;
};

inline SpatialGrid::SpatialGrid()
    : cellHeight(100.0f), minWorldY(-500.0f), maxWorldY(500.0f), numCells(0) {
    EnsureCellCount();
}

inline SpatialGrid::SpatialGrid(float cellHeight, float minY, float maxY)
    : cellHeight(cellHeight), minWorldY(minY), maxWorldY(maxY), numCells(0) {
    EnsureCellCount();
}

inline void SpatialGrid::SetWorldBounds(float minY, float maxY) {
    minWorldY = minY;
    maxWorldY = maxY;
    EnsureCellCount();
}

inline void SpatialGrid::SetCellHeight(float height) {
    cellHeight = height;
    EnsureCellCount();
}

inline void SpatialGrid::Clear() {
    for (auto& cell : platformCells) cell.clear();
    for (auto& cell : obstacleCells) cell.clear();
    for (auto& cell : checkpointCells) cell.clear();
    for (auto& cell : enemyCells) cell.clear();
    for (auto& cell : bulletCells) cell.clear();
}

inline int SpatialGrid::GetCellIndex(float y) const {
    if (numCells <= 0) return 0;
    int index = static_cast<int>((y - minWorldY) / cellHeight);
    return static_cast<int>(AEClamp(static_cast<f32>(index), static_cast<f32>(0), static_cast<f32>(numCells - 1)));
}

inline void SpatialGrid::EnsureCellCount() {
    numCells = static_cast<int>((maxWorldY - minWorldY) / cellHeight) + 1;
    platformCells.resize(numCells);
    obstacleCells.resize(numCells);
    checkpointCells.resize(numCells);
    enemyCells.resize(numCells);
    bulletCells.resize(numCells);
}

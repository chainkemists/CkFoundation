#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    // Chamfer step costs, in the transform's internal integer units: 3 across an edge, 4 across a
    // corner. The pair approximates Euclidean distance to within a few percent while keeping every
    // intermediate value an exact integer, so two bakes of the same input agree bit for bit.
    inline constexpr int32 kChamferOrthogonalCost = 3;
    inline constexpr int32 kChamferDiagonalCost = 4;

    /** Chamfer distance between two lattice offsets, in the same internal units. */
    CKGROUNDNAV_API auto
    Get_ChamferDistance(
        int32 InDeltaX,
        int32 InDeltaY) -> int32;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * How much room an agent standing on each cell has, in unreal units.
     *
     * This is the value that lets ONE bake serve every agent size: a query admits a cell by testing
     * its own radius against this number, instead of the bake eroding the field per radius.
     *
     * A cell with no walkable span on its layer reads zero, and so does anything outside the field.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ClearanceField
    {
    public:
        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        float _CellSizeUu = 0.0f;

        // _LayerCount planes of _SizeX * _SizeY, layer-major.
        TArray<float> _Cells;

    public:
        auto Get_IsValidCell(int32 InX, int32 InY, int32 InLayer) const -> bool
        {
            return InX >= 0 && InY >= 0 && InLayer >= 0 &&
                   InX < _SizeX && InY < _SizeY && InLayer < _LayerCount;
        }

        auto Get_ClearanceAt(int32 InX, int32 InY, int32 InLayer) const -> float
        {
            return Get_IsValidCell(InX, InY, InLayer)
                ? _Cells[(InLayer * _SizeX * _SizeY) + (InY * _SizeX) + InX]
                : 0.0f;
        }

        auto Get_MaxClearance() const -> float;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Distance from every walkable cell to the nearest place an agent standing there cannot step to,
     * per layer.
     *
     * A two-pass chamfer sweep (Borgefors 1986). A neighbour counts as ground only where the
     * connection field links the two cells; a neighbour that is walkable but NOT linked — the top
     * of a wall or a crate, the far side of a ledge, another storey's floor that layer extraction
     * packed into this layer — is an obstacle exactly as a hole is, because the body cannot step
     * onto it and its side is a wall. Occupancy alone would read the floor beside every solid as
     * open right up to it, and admit a body flush against a wall it cannot pass.
     *
     * For this metric the sweep is exact: every cell's nearest obstacle — a hole or an unlinked
     * edge — is reached along a monotone path of linked steps, because any path that crosses an
     * unlinked edge has that edge nearer than whatever lies beyond it. A brute-force reference over
     * holes therefore agrees cell for cell wherever every walkable neighbour is linked, and a
     * solid's neighbouring cells read exactly one cell size.
     *
     * THE FIELD BORDER COUNTS AS BLOCKED. Without that a fully walkable field would have unbounded
     * interior clearance and the number would mean nothing. The consequence is that clearance near
     * the border reads short, which is correct for a standalone field and is why a tiled bake must
     * rasterize a halo beyond each tile rather than computing this per tile in isolation.
     *
     * A crossing that changes layer leaves this layer at the crossing, so the cells before it read
     * a false pinch on this layer; that is a known limit of per-layer sweeping, not of the gating.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoCompute_Clearance(
        const FCk_GroundNav_LayerField&      InLayers,
        const FCk_GroundNav_ConnectionField& InConnections,
        float                                InCellSizeUu,
        FCk_GroundNav_ClearanceField&        OutClearance) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------

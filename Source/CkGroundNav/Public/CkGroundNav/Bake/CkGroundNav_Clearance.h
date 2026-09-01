#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"

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
     * Distance from every walkable cell to the nearest cell an agent cannot stand on, per layer.
     *
     * A two-pass chamfer sweep (Borgefors 1986), which for this metric is exact: the result equals
     * the minimum chamfer distance to any blocked cell, so a brute-force reference over the same
     * metric must agree cell for cell rather than approximately.
     *
     * THE FIELD BORDER COUNTS AS BLOCKED. Without that a fully walkable field would have unbounded
     * interior clearance and the number would mean nothing. The consequence is that clearance near
     * the border reads short, which is correct for a standalone field and is why a tiled bake must
     * rasterize a halo beyond each tile rather than computing this per tile in isolation.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoCompute_Clearance(
        const FCk_GroundNav_LayerField& InLayers,
        float                           InCellSizeUu,
        FCk_GroundNav_ClearanceField&   OutClearance) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------

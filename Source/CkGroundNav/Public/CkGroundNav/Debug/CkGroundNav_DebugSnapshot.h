#pragma once

#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Why a snapshot looks the way it does.
     *
     * FAILURE IS A STATUS, NEVER AN EMPTY SCENE. A viewer that cannot tell "nothing is walkable here"
     * from "the bake never ran" will report a hole in the world as confidently as it reports a floor.
     */
    enum class EDebugSnapshotStatus : uint8
    {
        NeverBuilt,
        BackendUnavailable,
        NoGeometryInRegion,
        Failed,
        Current
    };

    CKGROUNDNAV_API auto
    Get_StatusName(
        EDebugSnapshotStatus InStatus) -> const TCHAR*;

    // ----------------------------------------------------------------------------------------------------------------

    /** One walkable cell, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugCell
    {
    public:
        FVector _SurfaceCentre = FVector::ZeroVector;

        float _ClearanceUu = 0.0f;

        int32 _LayerIndex = 0;
        int32 _PlateIndex = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One merged plate, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugPlate
    {
    public:
        FBox _Bounds = FBox{ForceInit};

        int32 _LayerIndex = 0;

        float _HeightRangeUu = 0.0f;
        float _MaxPlaneResidualUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything a viewer needs to draw one bake, and nothing that could outlive it.
     *
     * VALUE-ONLY BY CONSTRUCTION: no world, no actor, no ECS handle, no registry, no span field, no
     * pointer back to anything that produced it. A viewer holding this can draw it a frame later, a
     * second later, or after the world it came from has been torn down.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugSnapshot
    {
    public:
        EDebugSnapshotStatus _Status = EDebugSnapshotStatus::NeverBuilt;

        FBox _Region = FBox{ForceInit};

        float _CellSizeUu = 0.0f;

        int32 _LayerCount = 0;
        int32 _SpanCount = 0;
        int32 _WalkableCellCount = 0;
        int32 _DroppedTriangleCount = 0;
        int32 _SourceTriangleCount = 0;

        float _MaxClearanceUu = 0.0f;

        // Plate diagnostics, surfaced so the merge tunables can be judged from a real level rather
        // than from a fixture.
        float _CollapseRatio = 0.0f;
        float _MaxPlaneResidualUu = 0.0f;
        float _MaxPlateHeightRangeUu = 0.0f;

        int32 _RejectedCellCount = 0;

        double _BakeMilliseconds = 0.0;

        TArray<FCk_GroundNav_DebugCell> _Cells;
        TArray<FCk_GroundNav_DebugPlate> _Plates;

        // Cells the rasterizer accepted on slope but the walkability filters then demoted. Drawing
        // these is the only way to see what a filter is COSTING you: an over-tight ledge sensitivity
        // and a genuinely unwalkable world produce the same walkable set, and differ only here.
        TArray<FCk_GroundNav_DebugCell> _RejectedCells;

        // Set when the cell list was capped. The counts above stay TRUE totals, so a viewer reports
        // what the bake found rather than what it managed to draw.
        bool _CellsWereTruncated = false;

    public:
        auto Get_IsDrawable() const -> bool { return _Status == EDebugSnapshotStatus::Current; }

        auto Get_PlateCount() const -> int32 { return _Plates.Num(); }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Record the cells the walkability filters demoted, by diffing the span field either side of
     * them. Both fields must be the same lattice — the caller holds the copy it took before filtering.
     */
    CKGROUNDNAV_API auto
    Do_RecordRejectedCells(
        const FCk_GroundNav_SpanField& InBeforeFilter,
        const FCk_GroundNav_SpanField& InAfterFilter,
        FCk_GroundNav_DebugSnapshot&   InOutSnapshot) -> void;

    /**
     * Copy finished bake products into a standalone snapshot.
     *
     * Every value is read out here and nothing is retained, which is what makes the boundary a copy
     * rather than a view. InMaxCells caps the per-cell list only — the reported counts stay exact.
     */
    CKGROUNDNAV_API auto
    Make_DebugSnapshot(
        const FCk_GroundNav_SpanField&      InSpans,
        const FCk_GroundNav_LayerField&     InLayers,
        const FCk_GroundNav_ClearanceField& InClearance,
        const FCk_GroundNav_PlateField&     InPlates,
        const FBox&                         InRegion,
        int32                               InMaxCells) -> FCk_GroundNav_DebugSnapshot;
}

// --------------------------------------------------------------------------------------------------------------------

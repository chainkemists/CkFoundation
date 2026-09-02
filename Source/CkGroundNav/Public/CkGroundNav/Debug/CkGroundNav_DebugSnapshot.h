#pragma once

#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_Portals.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"

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

    /** One crossing between two plates, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugPortal
    {
    public:
        FVector _MinEnd = FVector::ZeroVector;
        FVector _MaxEnd = FVector::ZeroVector;

        float _TraversalClearanceUu = 0.0f;

        // Whether the crossing changes floor. Worth its own flag rather than a plate lookup at draw
        // time: a portal between layers is the case a viewer most needs to pick out of a flat field.
        bool _IsCrossLayer = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One tile of a field, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugTile
    {
    public:
        FBox _Bounds = FBox{ForceInit};

        int32 _PlateCount = 0;
        int32 _WalkableCellCount = 0;

        // Drawn differently rather than omitted. A tile nothing is known about and a tile with no floor
        // in it look identical if only the built ones are shown, and they are the two things a viewer
        // most needs to tell apart.
        bool _IsBuilt = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One crossing between two tiles, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugSeam
    {
    public:
        FVector _MinEnd = FVector::ZeroVector;
        FVector _MaxEnd = FVector::ZeroVector;

        float _TraversalClearanceUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One Solid body whose mesh is not closed, already in world space. The viewer's loudest element:
     * the ground under such a body is not trustworthy and a developer has to go and fix the asset.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugOpenBody
    {
    public:
        FString _Description;

        FBox _Bounds = FBox{ForceInit};

        int32 _TriangleCount = 0;
        int32 _OpenEdgeCount = 0;

        // Two per recorded edge; capped at bake time, so Num()/2 may be below _OpenEdgeCount.
        TArray<FVector> _OpenEdgePoints;
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

        // The lattice the counts below are quantities OF. Reported rather than left implicit
        // because every per-cell number is bounded by _LayerCount * _LatticeSizeX * _LatticeSizeY,
        // and a count that exceeds that bound is the only way to see it is being mis-attributed.
        int32 _LatticeSizeX = 0;
        int32 _LatticeSizeY = 0;

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

        // Heap bytes of the published product (a field's tiles and arrays; a region bake's equivalent),
        // so the summary reports a cost and not just a count.
        int64 _AllocatedBytes = 0;

        // Open Solid bodies the closure check found. Drawn in EVERY mode and printed at the top of the
        // summary: this is the one thing a viewer must never let a developer miss.
        TArray<FCk_GroundNav_DebugOpenBody> _OpenBodies;

        TArray<FCk_GroundNav_DebugCell> _Cells;
        TArray<FCk_GroundNav_DebugPlate> _Plates;
        TArray<FCk_GroundNav_DebugPortal> _Portals;

        // Empty for a single-region bake, populated for a field bake. A viewer draws what is there
        // rather than being told which kind of bake it is looking at.
        TArray<FCk_GroundNav_DebugTile> _Tiles;
        TArray<FCk_GroundNav_DebugSeam> _Seams;

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

        auto Get_PortalCount() const -> int32 { return _Portals.Num(); }

        auto Get_TileCount() const -> int32 { return _Tiles.Num(); }

        auto Get_SeamCount() const -> int32 { return _Seams.Num(); }

        auto Get_BuiltTileCount() const -> int32;

        auto Get_OpenBodyCount() const -> int32 { return _OpenBodies.Num(); }

        /** The tightest crossing in the field, and the first number to read when a body that ought to
         *  fit somewhere cannot get there. Zero when there are no portals at all. */
        auto Get_NarrowestPortalUu() const -> float;
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
    /**
     * Copy a whole published field into a standalone snapshot.
     *
     * Walks every built tile and flattens its cells, plates and crossings into the same shape a
     * single-region bake produces, so every existing draw mode works over a tiled field unchanged —
     * and adds the two things only a field has: its tiles, and the crossings between them.
     *
     * Everything is read out here and nothing is retained. The field may be rebuilt or dropped the
     * moment this returns.
     */
    CKGROUNDNAV_API auto
    Make_DebugSnapshotFromField(
        const FCk_GroundNav_Field& InField,
        int32                      InMaxCells) -> FCk_GroundNav_DebugSnapshot;

    CKGROUNDNAV_API auto
    Make_DebugSnapshot(
        const FCk_GroundNav_SpanField&      InSpans,
        const FCk_GroundNav_LayerField&     InLayers,
        const FCk_GroundNav_ClearanceField& InClearance,
        const FCk_GroundNav_PlateField&     InPlates,
        const FCk_GroundNav_PortalField&    InPortals,
        const FBox&                         InRegion,
        int32                               InMaxCells) -> FCk_GroundNav_DebugSnapshot;
}

// --------------------------------------------------------------------------------------------------------------------

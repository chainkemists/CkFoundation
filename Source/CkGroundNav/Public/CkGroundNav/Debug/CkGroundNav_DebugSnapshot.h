#pragma once

#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
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

        // The epoch this tile was last baked under. Carried per tile rather than once per field
        // because a local repair moves only the tiles it re-baked: the ones still holding an older
        // epoch are the ground the repair carried across untouched, and without this the two are
        // indistinguishable in a view that shows every tile alike.
        int64 _Epoch = 0;

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

    /** One run of plate edge that nothing crosses, already in world space. */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugBoundary
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        // The left perpendicular of (End - Start): into the plate the run bounds.
        FVector2D _InwardNormalXY = FVector2D::ZeroVector;

        int32 _LayerIndex = 0;

        // Whether the run lies on a TILE rim rather than inside one. Worth its own flag rather than a
        // lookup at draw time: a rim run is a wall only until the neighbouring tile is baked, and a
        // viewer that drew the two alike would report unbaked ground as a wall.
        bool _IsTileRim = false;
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
     * One area markup a volume holds, reduced to what a viewer draws and what a reader needs to judge
     * it, already in world space.
     *
     * The markup ENTITY is deliberately absent, and the liveness answer is a captured value rather
     * than something re-asked at draw time: this honours the same copy boundary every other member of
     * a snapshot does, so a view drawn a frame later reports what was true when it was captured
     * instead of touching a registry that may have moved on.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugMarkup
    {
    public:
        FBox _Bounds = FBox{ForceInit};

        // The tag's name rather than the FGameplayTag, for the same reason the entity is absent: a
        // snapshot holds values, and a name is what the viewer prints.
        FName _AreaTagName;

        int32 _RecordId = INDEX_NONE;

        float _CostMultiplier = 1.0f;

        // The epoch the record was submitted against. A record stamped behind the field's current
        // epoch is the shape of a paint the bake has not caught up with.
        int64 _RequestedAtEpoch = 0;

        ECk_GroundNav_MarkupKind _Kind = ECk_GroundNav_MarkupKind::Walkability;

        bool _IsEnabled = true;

        // Whether the neutral facade reported the paint as reaching the surface. Drawn apart from
        // enabled: a record the volume holds and the bake has not applied yet is not a record the
        // author disabled, and treating the two alike hides the whole window this exists to show.
        bool _IsLive = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One navigation link a published field resolved, reduced to what a viewer draws and what a reader
     * needs to judge it, already in world space.
     *
     * The link ENTITY is deliberately absent and the liveness answer is a captured value, exactly as
     * for a markup: a view drawn a frame later reports what was true when it was captured instead of
     * touching a registry that may have moved on. No handle, no world, no field pointer.
     *
     * The endpoints are the AUTHORED points rather than what they resolved to, so a link whose end
     * found no ground still draws where its author put it - which is the whole of what an unresolved
     * end is a report about.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugLink
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        // The tags' names rather than the FGameplayTags, for the same reason the markup value carries a
        // name: a snapshot holds values, and a name is what the viewer prints.
        FName _AreaTagName;
        FName _UserTypeTagName;

        int32 _Id = INDEX_NONE;

        // Flat plate indices, INDEX_NONE for an end that resolved to nothing. Valid only against the
        // field they were derived on, which is why they are read beside the status and never alone.
        int32 _StartFlatPlate = INDEX_NONE;
        int32 _EndFlatPlate = INDEX_NONE;

        float _CostMultiplierForward = 1.0f;
        float _CostMultiplierBackward = 1.0f;

        float _ClearanceUu = FCk_GroundNav_LinkRecord::kAdmitsAnyAgentClearanceUu;

        ECk_GroundNav_LinkDirection _Direction = ECk_GroundNav_LinkDirection::Bidirectional;

        // Per end, because the two ends fail independently and a link with one end over unbaked ground
        // is a different report from one with both ends over a hole.
        ECk_NavSurface_QueryStatus _StartStatus = ECk_NavSurface_QueryStatus::NoSurface;
        ECk_NavSurface_QueryStatus _EndStatus = ECk_NavSurface_QueryStatus::NoSurface;

        bool _Enabled = true;

        // Whether the volume reported the link as reaching the published ground. Drawn apart from
        // enabled for the same reason a markup's liveness is: a record the bake has not caught up with
        // is not a record the author switched off.
        bool _Live = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One agent's cached path corridor and the two epochs an invalidation decision is made on,
     * already in world space.
     *
     * The path ENTITY is absent for the same reason a markup's is, and both epochs are captured
     * values rather than something re-asked at draw time: a view drawn a frame later reports what
     * was true when it was captured instead of touching a registry that may have moved on.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_DebugCorridor
    {
    public:
        // ALREADY inflated by _InflationUu - the very box the invalidator intersects a rebuild against,
        // so a viewer is looking at the test rather than at an approximation of it.
        FBox _Bounds = FBox{ForceInit};

        // How the agent printed at capture. A name is a value; the handle it came from is not.
        FString _PathName;

        float _InflationUu = 0.0f;

        int64 _CorridorEpoch = 0;

        // The epoch of the published field covering the corridor's centre. Zero and _HasField false
        // where the world has none: a corridor that outlived its field is not a corridor at epoch 0.
        int64 _FieldEpoch = 0;

        bool _HasField = false;

        bool _RepathRequired = false;
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

        // Every plate edge run no crossing covers, plus the rim runs the field resolved against each
        // tile's neighbours. Empty for a single-region bake: the boundary is derived per TILE, so a
        // bake that produced no tiles has none to report.
        TArray<FCk_GroundNav_DebugBoundary> _Boundary;

        // Cells the rasterizer accepted on slope but the walkability filters then demoted. Drawing
        // these is the only way to see what a filter is COSTING you: an over-tight ledge sensitivity
        // and a genuinely unwalkable world produce the same walkable set, and differ only here.
        TArray<FCk_GroundNav_DebugCell> _RejectedCells;

        // The area markup the world's volumes hold, when a caller collected it. Empty otherwise, and
        // a viewer draws what is there rather than being told whether collection was asked for.
        TArray<FCk_GroundNav_DebugMarkup> _Markups;

        // The links the world's published fields resolved, when a caller collected them. Empty
        // otherwise, and a viewer draws what is there rather than being told whether collection was
        // asked for.
        TArray<FCk_GroundNav_DebugLink> _Links;

        // The path corridors the world's agents hold, when a caller collected them. Empty otherwise.
        TArray<FCk_GroundNav_DebugCorridor> _Corridors;

        // The ground each of the world's fields last published its news ABOUT. One box per field
        // rather than their union: two volumes republishing in the same window changed two places,
        // never the span between them.
        TArray<FBox> _ChangedBounds;

        // Dirty ground a volume has accumulated that no repair has opened for yet. Invalid when
        // nothing is pending, which is a different thing from an empty box at the origin.
        FBox _PendingDirtyBounds = FBox{ForceInit};

        // The box the OPEN repair fixed its tile set from. Held apart from the pending box because
        // the two are consecutive states of the same ground and a viewer that drew them alike could
        // not tell a repair that has started from one still waiting to.
        FBox _RepairDirtyBounds = FBox{ForceInit};

        // The tiles that repair is re-baking, indexed into the VOLUME's field.
        TArray<int32> _RepairTileIndices;

        // Those same tiles placed in world space at capture. The indices above address the volume's
        // lattice, and a snapshot's own tiles come from a separate debug bake, so nothing at draw
        // time could place them.
        TArray<FBox> _RepairTileBounds;

        bool _RepairInProgress = false;

        // Set when the cell list was capped. The counts above stay TRUE totals, so a viewer reports
        // what the bake found rather than what it managed to draw.
        bool _CellsWereTruncated = false;

    public:
        auto Get_IsDrawable() const -> bool { return _Status == EDebugSnapshotStatus::Current; }

        auto Get_PlateCount() const -> int32 { return _Plates.Num(); }

        auto Get_PortalCount() const -> int32 { return _Portals.Num(); }

        auto Get_TileCount() const -> int32 { return _Tiles.Num(); }

        auto Get_SeamCount() const -> int32 { return _Seams.Num(); }

        auto Get_BoundaryCount() const -> int32 { return _Boundary.Num(); }

        auto Get_BuiltTileCount() const -> int32;

        auto Get_RepairTileCount() const -> int32 { return _RepairTileIndices.Num(); }

        /** The highest epoch any tile carries, which is the one the latest publish stamped: a repair
         *  moves only the tiles it re-baked, so the tiles at this epoch are exactly that publish's
         *  news. Zero for a snapshot with no tiles. */
        auto Get_NewestTileEpoch() const -> int64;

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

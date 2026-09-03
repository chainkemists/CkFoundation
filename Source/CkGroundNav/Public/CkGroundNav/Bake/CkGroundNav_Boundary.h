#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_Portals.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One run of plate edge that nothing crosses: a wall, a drop, the rim of a hole.
     *
     * WINDING IS FIXED AT BAKE: walking from Start to End the plate's interior is on the LEFT, so
     * every consumer reads the inward normal the same way and none re-derives it. Endpoints lie on
     * the cell edge, in world space, at the surface height of the run's first and last cell; the cell
     * run and the side are kept so a query can reason in lattice terms without decoding them.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_BoundarySegment
    {
    public:
        int32 _PlateIndex = FCk_GroundNav_Plate::kNoPlate;
        int32 _LayerIndex = 0;

        // The face the run lies on: 0 east (+X), 1 north (+Y), 2 west, 3 south.
        int32 _Side = 0;

        // The run of edge cells, inclusive, in the winding order.
        FIntPoint _FromCell = FIntPoint::ZeroValue;
        FIntPoint _ToCell = FIntPoint::ZeroValue;

        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        // The left perpendicular of (End - Start): into the plate.
        FVector2D _InwardNormalXY = FVector2D::ZeroVector;

    public:
        auto Get_CellCount() const -> int32
        {
            return FMath::Abs(_ToCell.X - _FromCell.X) + FMath::Abs(_ToCell.Y - _FromCell.Y) + 1;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Every boundary run of one tile, with a coarse index over them.
     *
     * Runs on the tile's own outer rim are NOT among the segments: whether such an edge is a wall or
     * a crossing depends on the neighbouring tile, so they are kept aside as candidates and resolved
     * when the field is composed, exactly as seam portals are.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_BoundaryField
    {
    public:
        static constexpr int32 kBucketCells = 8;

    public:
        TArray<FCk_GroundNav_BoundarySegment> _Segments;

        // Runs on the tile's outer rim, decided at composition.
        TArray<FCk_GroundNav_BoundarySegment> _EdgeCandidates;

        int32 _BucketsX = 0;
        int32 _BucketsY = 0;

        // Segment indices per bucket; a run is listed under every bucket its cells touch.
        TArray<TArray<int32>> _Buckets;

    public:
        auto Get_BucketCoord(int32 InCellX, int32 InCellY) const -> FIntPoint
        {
            return FIntPoint{InCellX / kBucketCells, InCellY / kBucketCells};
        }

        auto Get_IsValidBucket(const FIntPoint& InBucket) const -> bool
        {
            return InBucket.X >= 0 && InBucket.Y >= 0 && InBucket.X < _BucketsX && InBucket.Y < _BucketsY;
        }

        auto Get_Bucket(const FIntPoint& InBucket) const -> TConstArrayView<int32>
        {
            return Get_IsValidBucket(InBucket)
                ? TConstArrayView<int32>{_Buckets[(InBucket.Y * _BucketsX) + InBucket.X]}
                : TConstArrayView<int32>{};
        }

        auto Get_AllocatedSize() const -> SIZE_T;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** What the lattice a tile publishes looks like, as the boundary derivation needs it. */
    struct CKGROUNDNAV_API FCk_GroundNav_BoundaryLattice
    {
    public:
        FVector _Origin = FVector::ZeroVector;

        float _CellSizeUu = 0.0f;

        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        // Layer-major, as the tile stores it.
        const TArray<float>* _SurfaceZ = nullptr;

    public:
        auto Get_SurfaceZ(int32 InX, int32 InY, int32 InLayer) const -> float
        {
            return (*_SurfaceZ)[(InLayer * _SizeX * _SizeY) + (InY * _SizeX) + InX];
        }
    };

    /**
     * One segment for a run of edge cells on one side of one plate, endpoints and normal derived from
     * the run alone. The single place the winding lives; composition and the bake both call it.
     */
    CKGROUNDNAV_API auto
    Make_BoundarySegment(
        const FCk_GroundNav_BoundaryLattice& InLattice,
        int32                                InPlateIndex,
        int32                                InLayerIndex,
        int32                                InSide,
        const FIntPoint&                     InFromCell,
        const FIntPoint&                     InToCell) -> FCk_GroundNav_BoundarySegment;

    /**
     * Derive every plate edge run no portal covers.
     *
     * Runs on the tile's outer rim go to _EdgeCandidates. Deterministic order: plates ascending,
     * sides 0..3, runs in winding order. Bills one probe per edge cell visited.
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoDerive_Boundary(
        const FCk_GroundNav_BoundaryLattice& InLattice,
        const FCk_GroundNav_PlateField&      InPlates,
        const FCk_GroundNav_PortalField&     InPortals,
        FCk_GroundNav_BoundaryField&         OutBoundary,
        int32&                               InOutProbes) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

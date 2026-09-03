#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkGroundNav/Bake/CkGroundNav_Boundary.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkGroundNav_QueryTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Value types every ground query speaks. Free of world, registry and entity concepts, exactly like the
// bake: a query is math over a published field, and the ECS shell above it only resolves which field.
//
// Statuses are the provider-neutral ones from CkNavigation. Unbuilt and NoSurface are never
// conflated at any entry point: the first is a place nothing is known about, the second a place with
// nowhere to stand, and a consumer defers on one and gives up on the other.
// --------------------------------------------------------------------------------------------------------------------

/**
 * Whether a region of the world has a usable field under it.
 *
 * OutsideField is distinct from Unbuilt on purpose. Ground no volume covers will never be built, so
 * a consumer that waits on Unbuilt would wait forever there; ground a volume covers but has not
 * baked yet is worth waiting for. Building is answered only by a volume with a build in flight — a
 * published field is immutable and never half-built, so it never reports it.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_RegionStatus : uint8
{
    // Every tile the region touches is built.
    Built,

    // Some tiles the region touches are built and some are not.
    PartiallyBuilt,

    // A build covering the region is in flight.
    Building,

    // The region lies over tiles that have not been baked, and no build is in flight.
    Unbuilt,

    // No tile covers the region at all.
    OutsideField
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_RegionStatus);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Where on the field a surface answer lives. Integer identity only, so a result can be held,
     * compared and drawn after the field it came from has been rebuilt; it is valid only against
     * the field epoch it was answered from.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceRef
    {
    public:
        int32 _TileIndex = INDEX_NONE;
        int32 _LayerIndex = INDEX_NONE;

        // Tile-local, like every index a tile carries.
        int32 _CellX = INDEX_NONE;
        int32 _CellY = INDEX_NONE;
        int32 _PlateIndex = FCk_GroundNav_Plate::kNoPlate;

    public:
        auto Get_IsValid() const -> bool
        {
            return _TileIndex != INDEX_NONE && _PlateIndex != FCk_GroundNav_Plate::kNoPlate;
        }

        auto operator==(const FCk_GroundNav_SurfaceRef&) const -> bool = default;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What a query knows about the body asking.
     *
     * Radius only. The field was baked for a profile (height, slope, step) and answers for that
     * profile alone; radius is the one dimension a single bake serves every value of, by testing it
     * against per-cell clearance. Zero admits every walkable cell.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_QueryAgent
    {
    public:
        float _RadiusUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What a query cost, in the bake's own unit: one innermost cell read is one probe. Deterministic
     * for a given field and query, which is what lets a test assert one query is cheaper than
     * another rather than time them.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_QueryCost
    {
    public:
        int32 _CellsRead = 0;
        int32 _TilesTouched = 0;

        // Whether the query's footprint reached a tile that is not built. Carried so the status can
        // say Unbuilt rather than NoSurface when nothing qualified — the answer may be in that tile.
        bool _TouchedUnbuiltTile = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Find the nearest walkable surface to a point.
     *
     * The search volume is a box: a horizontal half-extent in XY and two separate vertical reaches,
     * because the two directions mean different things to a grounded agent — a floor a little way
     * below is where it is standing, a floor far above is another storey.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ProjectionQuery
    {
    public:
        FVector _Location = FVector::ZeroVector;

        float _HorizontalExtentUu = 0.0f;
        float _UpExtentUu = 0.0f;
        float _DownExtentUu = 0.0f;

        ECk_NavSurface_ProjectionMode _Mode = ECk_NavSurface_ProjectionMode::Closest;

        FCk_GroundNav_QueryAgent _Agent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_ProjectionResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        // The query point moved onto the surface: its XY clamped into the answering cell, at that
        // cell's surface height. Meaningful only on Success.
        FVector _Location = FVector::ZeroVector;

        FVector _SurfaceNormal = FVector::UpVector;

        FCk_GroundNav_SurfaceRef _Surface;

        float _ClearanceUu = 0.0f;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Is this exact position on walkable ground: the column lookup without any horizontal search,
     * admitting a layer whose surface is within the vertical tolerance of the query height.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_IsNavigableQuery
    {
    public:
        FVector _Location = FVector::ZeroVector;

        float _VerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_IsNavigableResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FCk_GroundNav_SurfaceRef _Surface;

        float _SurfaceZUu = 0.0f;
        float _ClearanceUu = 0.0f;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything the field knows about the ground at one surface, read per PLATE: the normal is
     * recovered from the plate's own heights, and policy is stored at plate level so a query never
     * pays per cell for what does not vary per cell.
     *
     * Area tags and the cost multiplier come from the plate's own label, which is what the area markup
     * over it stamped. A plate no record covers carries INDEX_NONE and the identity multiplier.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceAttributes
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FCk_GroundNav_SurfaceRef _Surface;

        FVector _SurfaceNormal = FVector::UpVector;

        FGameplayTagContainer _AreaTags;

        float _CostMultiplier = 1.0f;

        float _ClearanceUu = 0.0f;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };
    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Move a body along walkable ground from a start that is on the surface toward a target, going
     * as far as the ground allows and never further.
     *
     * The start is resolved to a surface first: a layer whose surface lies within the start tolerance
     * of the start height. The start cell is admitted whatever its clearance — the body is already
     * standing there, and a walk may leave a tight spot but never enter one.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceWalkQuery
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _Target = FVector::ZeroVector;

        float _StartVerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Where a walk ended. On Success the location is ON the walkable set by construction: its XY
     * lies inside the answering cell's square and its height is that cell's surface. That is the
     * containment guarantee grounded agents stand on, and it is asserted, never clamped into being.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceWalkResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FVector _Location = FVector::ZeroVector;

        FCk_GroundNav_SurfaceRef _Surface;

        bool _ReachedTarget = false;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The instrumented counters a walk reports, so a test asserts on what the walk DID, not on time. */
    struct CKGROUNDNAV_API FCk_GroundNav_SurfaceWalkDiagnostics
    {
    public:
        int32 _CellsStepped = 0;
        int32 _BlockedSteps = 0;
        int32 _SlideCount = 0;
        int32 _PortalCrossings = 0;
        int32 _SeamCrossings = 0;

        bool _TookPlateEarlyOut = false;
        bool _HitIterationBound = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Can a body walk the straight segment from start to end without leaving walkable ground.
     *
     * The optional cost cap accumulates each cell's traversal cost multiplier times the length walked
     * inside it; zero means no cap. That is what lets a caller ask "walkable AND cheap enough" in one
     * pass rather than two.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_RaycastQuery
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        float _StartVerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        float _MaxCost = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Success means the whole segment is walkable. Blocked means the ray stopped: at the first cell
     * boundary the body could not cross, or where the accumulated cost passed the cap — the flag says
     * which. The hit normal is the crossed edge's normal facing back along the ray, in XY.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_RaycastResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FVector _HitLocation = FVector::ZeroVector;
        FVector _HitNormal = FVector::ZeroVector;

        FCk_GroundNav_SurfaceRef _LastSurface;

        float _AccumulatedCost = 0.0f;

        bool _StoppedOnCost = false;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsClear() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };
    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The walls around a point: every boundary run within a radius on the surface the point stands
     * on, within a vertical window of its height so another storey's rim does not count as a wall.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_BoundaryQuery
    {
    public:
        FVector _Location = FVector::ZeroVector;

        float _RadiusUu = 0.0f;

        float _VerticalWindowUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        // Zero means every run in range. Otherwise the nearest this many, nearest first.
        int32 _MaxSegments = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_ClosestBoundaryQuery
    {
    public:
        FVector _Location = FVector::ZeroVector;

        float _MaxRadiusUu = 0.0f;

        float _VerticalWindowUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_ClosestBoundaryResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FCk_GroundNav_BoundarySegment _Segment;

        FVector _ClosestPoint = FVector::ZeroVector;

        float _DistanceUu = 0.0f;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What a label comparison can say. It reads connectivity off the bake and nothing else, so it
     * can prove two points apart but never prove them joined for a particular body: clearance is
     * not in the label, and a doorway the two share may be too narrow for the body asking.
     */
    enum class ECk_GroundNav_Reachability : uint8
    {
        // Same component. A path may still fail on clearance; only the flood fill knows.
        PossiblyReachable,

        // Different components, both closed: no crossing joins them and none can appear without a
        // rebuild.
        Unreachable,

        // Different components, but one of them borders ground nobody has baked yet, so the
        // crossing that would join them may simply not have been looked at.
        Unknown_OpenComponent
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_ReachabilityQuery
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;

        float _VerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_ReachabilityResult
    {
    public:
        // Success when both ends resolved to a surface; otherwise the status of the end that failed
        // (the start first), and _Reachability is meaningless.
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        ECk_GroundNav_Reachability _Reachability = ECk_GroundNav_Reachability::Unreachable;

        FCk_GroundNav_SurfaceRef _StartSurface;
        FCk_GroundNav_SurfaceRef _EndSurface;

        // Crossings expanded to reach the verdict. A label comparison expands none: the field is
        // what makes that constant-time claim observable rather than asserted.
        int32 _ExpansionCount = 0;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One crossing between two plates of the whole field, oriented away from the plate it was
     * enumerated from. Plates are addressed FLAT (tile offset + tile-local index), which is the
     * index space the reachability labels already use.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Crossing
    {
    public:
        int32 _FromFlatPlate = INDEX_NONE;
        int32 _ToFlatPlate = INDEX_NONE;

        // Outward from the plate being left, as a direction index (Get_DirectionOffset).
        int32 _Direction = 0;

        // The interval on the shared cell line in world space, left and right as seen by a body
        // walking through it in _Direction.
        FVector _Left = FVector::ZeroVector;
        FVector _Right = FVector::ZeroVector;

        float _ClearanceUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_FloodQuery
    {
    public:
        FVector _Source = FVector::ZeroVector;

        float _VerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        // A crossing whose string-pulled distance exceeds this is never settled. Zero means no limit.
        float _MaxDistanceUu = 0.0f;

        // Zero means no limit.
        int32 _MaxExpansions = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A crossing the flood fill settled: where it was entered and how far that point is from the source. */
    struct CKGROUNDNAV_API FCk_GroundNav_FloodCrossing
    {
    public:
        FCk_GroundNav_Crossing _Crossing;

        // The point on the (inset) interval the shortest path passes through.
        FVector _EntryPoint = FVector::ZeroVector;

        // String-pulled from the source through every predecessor: the true walked distance, not a
        // sum of portal centres.
        double _DistanceUu = 0.0;

        // Index into the flood's settled crossings, INDEX_NONE for a crossing left from the source plate.
        int32 _Predecessor = INDEX_NONE;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_FloodResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        FCk_GroundNav_SurfaceRef _SourceSurface;
        int32 _SourceFlatPlate = INDEX_NONE;
        FVector _SourcePoint = FVector::ZeroVector;

        // In settle order, so a predecessor index always points earlier.
        TArray<FCk_GroundNav_FloodCrossing> _Crossings;

        // Per flat plate: every settled crossing that enters it, in settle order. Empty for a plate
        // the flood never reached. The source plate is reached at distance zero whether or not a
        // cycle later settles a crossing back into it, which is why Get_IsPlateReached tests it by
        // index.
        TArray<TArray<int32>> _PlateEntries;

        // Crossings popped from the frontier, including the one a distance limit or a stop predicate
        // then refused. Never more than _MaxExpansions when that is set.
        int32 _ExpansionCount = 0;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }

        auto Get_IsPlateReached(int32 InFlatPlate) const -> bool
        {
            return InFlatPlate == _SourceFlatPlate ||
                   (_PlateEntries.IsValidIndex(InFlatPlate) && _PlateEntries[InFlatPlate].Num() > 0);
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A generated point and where it stands, so attributes can be read without re-resolving it. */
    struct CKGROUNDNAV_API FCk_GroundNav_GeneratedPoint
    {
    public:
        FVector _Location = FVector::ZeroVector;

        FCk_GroundNav_SurfaceRef _Surface;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_PointsResult
    {
    public:
        ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoSurface;

        TArray<FCk_GroundNav_GeneratedPoint> _Points;

        // Random draws spent, accepted or not. A generator that had to reject is honest about it here.
        int32 _Attempts = 0;

        FCk_GroundNav_QueryCost _Cost;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_NavSurface_QueryStatus::Success; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_RandomPointsQuery
    {
    public:
        FVector _Origin = FVector::ZeroVector;

        float _RadiusUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        int32 _Count = 0;

        // The same seed on the same field epoch reproduces the same points.
        int32 _Seed = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_PathDistancePointsQuery
    {
    public:
        FVector _Origin = FVector::ZeroVector;

        float _MinDistanceUu = 0.0f;
        float _MaxDistanceUu = 0.0f;

        float _VerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        int32 _Count = 0;

        int32 _Seed = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FCk_GroundNav_GridPointsQuery
    {
    public:
        FBox _Bounds = FBox{ForceInit};

        float _SpacingUu = 0.0f;

        // Enabled: the lattice is phased to the field origin, so overlapping queries share positions.
        // Disabled: phased to the box's own minimum corner.
        ECk_EnableDisable _AlignToLattice = ECk_EnableDisable::Enable;

        FCk_GroundNav_QueryAgent _Agent;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkCore/Format/CkFormat.h"

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
     * Area tags and the cost multiplier are the shape of the answer, not yet its content: nothing in
     * the bake carries a traversal policy until runtime markup exists, so every plate answers no tags
     * and a multiplier of one. The fields are here so the consumer contract does not change when the
     * markup that gives them meaning arrives.
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
}

// --------------------------------------------------------------------------------------------------------------------

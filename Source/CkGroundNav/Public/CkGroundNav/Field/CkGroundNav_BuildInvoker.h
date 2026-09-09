#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /** A point source for invoker-driven ground-nav tile scope. Both radii are world-space XY distances. */
    struct CKGROUNDNAV_API FCk_GroundNav_BuildInvokerPoint
    {
    public:
        FVector _Location = FVector::ZeroVector;

        float _InnerRadiusUu = 0.0f;
        float _OuterRadiusUu = 0.0f;
    };

    /** A box source for invoker-driven ground-nav tile scope. Padding expands its XY retention box. */
    struct CKGROUNDNAV_API FCk_GroundNav_BuildInvokerBox
    {
    public:
        FBox _InnerBounds = FBox{ForceInit};

        float _OuterPaddingUu = 0.0f;
    };

    /**
     * ECS point-invoker data. The transform fragment owns the location, so moving this entity changes
     * scope without copying transform state into a second fragment.
     */
    struct CKGROUNDNAV_API FFragment_GroundNav_BuildInvoker
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNav_BuildInvoker);

    private:
        float _InnerGenerationRadiusUu = 0.0f;
        float _OuterRemovalRadiusUu = 0.0f;

    public:
        CK_PROPERTY(_InnerGenerationRadiusUu);
        CK_PROPERTY(_OuterRemovalRadiusUu);

    public:
        CK_DEFINE_CONSTRUCTORS(
            FFragment_GroundNav_BuildInvoker,
            _InnerGenerationRadiusUu,
            _OuterRemovalRadiusUu);
    };

    /** ECS box-invoker data. It is already world-space because it describes a volume source, not an entity pose. */
    struct CKGROUNDNAV_API FFragment_GroundNav_BuildInvokerVolume
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNav_BuildInvokerVolume);

    private:
        FBox _InnerBounds = FBox{ForceInit};
        float _OuterPaddingUu = 0.0f;

    public:
        CK_PROPERTY(_InnerBounds);
        CK_PROPERTY(_OuterPaddingUu);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_GroundNav_BuildInvokerVolume, _InnerBounds, _OuterPaddingUu);
    };

    /**
     * The deterministic scope decision for one owner lattice. Every array is a unique, row-major tile
     * index list. _Generate and _Retain describe the raw invoker unions; _Build and _Purge describe the
     * transition from the supplied currently-built set.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_BuildInvokerSelection
    {
    public:
        TArray<int32> _GenerateTileIndices;
        TArray<int32> _RetainTileIndices;
        TArray<int32> _BuildTileIndices;
        TArray<int32> _PurgeTileIndices;
    };

    /**
     * Compute one owner lattice's invoker scope without reading or mutating ECS or registry state.
     *
     * A point includes a tile when its closed XY AABB is no farther than the applicable radius. A box
     * includes a tile when its closed XY AABB intersects the applicable box. The desired set is
     * generation union (currently built intersect retention), so the outer set only preserves work
     * that already exists. Invalid lattice or currently-built input is refused and leaves OutSelection
     * untouched. Invalid descriptors are diagnosed and omitted, so one malformed ECS entity cannot
     * prevent valid invokers from determining the owner scope.
     */
    CKGROUNDNAV_API auto
    Request_ComputeBuildInvokerSelection(
        const FCk_GroundNav_FieldParams&                    InFieldParams,
        TConstArrayView<FCk_GroundNav_BuildInvokerPoint>    InPointInvokers,
        TConstArrayView<FCk_GroundNav_BuildInvokerBox>      InBoxInvokers,
        TConstArrayView<int32>                              InCurrentlyBuiltTileIndices,
        FCk_GroundNav_BuildInvokerSelection&                OutSelection) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------

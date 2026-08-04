#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoxelNav/Occluder/CkVoxelNavOccluder_Fragment.h"
#include "CkVoxelNav/Occluder/CkVoxelNavOccluder_Fragment_Data.h"

#include "CkVoxelNavOccluder_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoxelNavOccluder"))
class CKVOXELNAV_API UCk_Utils_VoxelNavOccluder_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoxelNavOccluder_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoxelNavOccluder);

public:
    /** Marks an entity as a dynamic obstacle that voxel volumes must keep up with.
     *
     *  Composed ON the entity itself rather than on a child, because the whole feature is a reading of that
     *  entity's transform - and the entity must already carry the Transform feature, which the setup
     *  processor ensures on.
     *
     *  This does NOT make the entity block anything. Occupancy comes from the geometry backend, so an
     *  occluder only occludes if it has a body the bake can see; the feature's job is to say WHEN and WHERE
     *  the volumes have to look again. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavOccluder",
              DisplayName="[Ck][VoxelNavOccluder] Add Occluder Feature")
    static FCk_Handle_VoxelNavOccluder
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VoxelNavOccluder_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavOccluder",
              DisplayName="[Ck][VoxelNavOccluder] Has Occluder Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** The world bounds the volumes were last told about. It lags the entity's live pose by up to one
     *  movement threshold, which is exactly what the threshold buys. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavOccluder",
              DisplayName="[Ck][VoxelNavOccluder] Get Tracked Bounds")
    static FBox
    Get_TrackedBounds(
        const FCk_Handle_VoxelNavOccluder& InOccluder);

    // How many times this occluder has emitted a dirty region. A tracked obstacle whose count never moves
    // is either not moving or moving below its threshold.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavOccluder",
              DisplayName="[Ck][VoxelNavOccluder] Get Times Dirtied")
    static int32
    Get_TimesDirtied(
        const FCk_Handle_VoxelNavOccluder& InOccluder);
};

// --------------------------------------------------------------------------------------------------------------------

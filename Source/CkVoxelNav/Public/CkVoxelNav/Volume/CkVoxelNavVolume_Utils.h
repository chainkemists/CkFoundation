#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

#include "CkVoxelNavVolume_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoxelNavVolume"))
class CKVOXELNAV_API UCk_Utils_VoxelNavVolume_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoxelNavVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoxelNavVolume);

public:
    // Add the volumetric-navigation feature to InOwner. Creates a child entity carrying the bake
    // params; FProcessor_VoxelNavVolume_Setup consumes its NeedsBuild tag on the next tick.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Add Volume Feature")
    static FCk_Handle_VoxelNavVolume
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_VoxelNavVolume_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Has Volume Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

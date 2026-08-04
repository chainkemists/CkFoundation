#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

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
    // Add the volumetric-navigation feature to InOwner. Creates a child entity carrying the bake params;
    // FProcessor_VoxelNavVolume_Setup consumes its NeedsSetup tag on the next tick and, unless the params
    // opted out, arms the first build from there.
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

public:
    /** Bakes the volume, resuming across as many ticks as the probe budget needs. A request arriving while
     *  a build is already underway is an idempotent no-op unless it forces a restart.
     *
     *  The completion delegate fires when the BUILD ends, not when the request is accepted — that is
     *  frames later, and it is the only outcome a caller can act on. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Request Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavVolume
    Request_Build(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Abandons an in-flight build. An already-published octree stays published. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Request Cancel Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavVolume
    Request_CancelBuild(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_CancelBuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // True once a bake has been published. A volume rebuilding over an existing bake still reads true —
    // the old octree stays live until the new one atomically replaces it.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Is Built")
    static bool
    Get_IsBuilt(
        const FCk_Handle_VoxelNavVolume& InVolume);

    // Bumps on every completed build. A path planned against an older epoch is stale and must replan.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Epoch")
    static int32
    Get_BuildEpoch(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Stage")
    static ECk_VoxelNav_BuildStage
    Get_BuildStage(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Progress")
    static float
    Get_BuildProgress(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Build Stats")
    static FCk_VoxelNav_BuildStats
    Get_BuildStats(
        const FCk_Handle_VoxelNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Num Layers")
    static int32
    Get_NumLayers(
        const FCk_Handle_VoxelNavVolume& InVolume);

    /** Is the finest cell containing InLocation navigable?
     *
     *  A volume with no published bake answers false for every point, as does a point outside the baked
     *  cube: the volume makes no claim about space it has not rasterized, and answering "free" there would
     *  invite a path straight out of the volume. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Get Is Point Free")
    static bool
    Get_IsPointFree(
        const FCk_Handle_VoxelNavVolume& InVolume,
        FVector InLocation);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Bind To OnBuildComplete")
    static FCk_Handle_VoxelNavVolume
    BindTo_OnBuildComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavVolume",
              DisplayName="[Ck][VoxelNavVolume] Unbind From OnBuildComplete")
    static FCk_Handle_VoxelNavVolume
    UnbindFrom_OnBuildComplete(
        UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

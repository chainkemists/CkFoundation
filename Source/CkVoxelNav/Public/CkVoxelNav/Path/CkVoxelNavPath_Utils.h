#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment.h"
#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment_Data.h"

#include "CkVoxelNavPath_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoxelNavPath"))
class CKVOXELNAV_API UCk_Utils_VoxelNavPath_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoxelNavPath_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoxelNavPath);

public:
    /** Give an entity the ability to plan volumetric paths. The fragments are stamped directly on InHandle
     *  rather than on a child entity: a path belongs to the agent that flies it, and every consumer that
     *  reads waypoints already holds the agent's handle. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Add Feature")
    static FCk_Handle_VoxelNavPath
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VoxelNavPath_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** Plan a route through the volume's baked free space. The search is synchronous inside the drain, so
     *  the completion delegate fires on the tick after the request — with Succeeded only when waypoints
     *  are readable. Every rejection (a volume that is not one, one that has not baked, endpoints outside
     *  the bake, an unreachable goal, a spent iteration budget) completes as Failed and fires
     *  OnPathFailed carrying which of those it was.
     *
     *  A successful search is refined before it is stored, per the request's own knobs — so the waypoints a
     *  caller reads are the refined ones, and the raw cell-centre path is not kept. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Request Find Path",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VoxelNavPath
    Request_FindPath(
        UPARAM(ref) FCk_Handle_VoxelNavPath& InPath,
        const FCk_Request_VoxelNavPath_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    /** Reports `Stale` for a Ready path whose volume has rebuilt since — including one whose volume is
     *  gone entirely. The stored status is never `Stale`: staleness belongs to the volume, and deriving it
     *  here is what keeps a rebuild from having to find every path that ever planned against it. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Status")
    static ECk_VoxelNav_PathStatus
    Get_Status(
        const FCk_Handle_VoxelNavPath& InPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Is Stale")
    static bool
    Get_IsStale(
        const FCk_Handle_VoxelNavPath& InPath);

    // The route to fly, as refined: the From position, the cells the search kept, then the To position.
    // Empty unless the path is Ready.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Waypoints")
    static TArray<FVector>
    Get_Waypoints(
        const FCk_Handle_VoxelNavPath& InPath);

    // How many waypoints the search produced before refinement. Against Get Refined Waypoint Count it is
    // what refinement bought, and it is the only trace of the raw path — that path is not kept.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Raw Waypoint Count")
    static int32
    Get_RawWaypointCount(
        const FCk_Handle_VoxelNavPath& InPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Refined Waypoint Count")
    static int32
    Get_RefinedWaypointCount(
        const FCk_Handle_VoxelNavPath& InPath);

    // Total length of the stored waypoints in uu, so a caller can compare two plans without walking them.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Path Length")
    static float
    Get_PathLengthUu(
        const FCk_Handle_VoxelNavPath& InPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Last Search Outcome")
    static ECk_VoxelNav_PathSearchOutcome
    Get_LastSearchOutcome(
        const FCk_Handle_VoxelNavPath& InPath);

    // The volume build epoch this path was planned against. Compare it with the volume's current epoch.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Get Planned Against Epoch")
    static int32
    Get_PlannedAgainstEpoch(
        const FCk_Handle_VoxelNavPath& InPath);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Bind To OnPathReady")
    static FCk_Handle_VoxelNavPath
    BindTo_OnPathReady(
        UPARAM(ref) FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Unbind From OnPathReady")
    static FCk_Handle_VoxelNavPath
    UnbindFrom_OnPathReady(
        UPARAM(ref) FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathReady& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Bind To OnPathFailed")
    static FCk_Handle_VoxelNavPath
    BindTo_OnPathFailed(
        UPARAM(ref) FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoxelNavPath",
              DisplayName="[Ck][VoxelNavPath] Unbind From OnPathFailed")
    static FCk_Handle_VoxelNavPath
    UnbindFrom_OnPathFailed(
        UPARAM(ref) FCk_Handle_VoxelNavPath& InPath,
        const FCk_Delegate_VoxelNavPath_OnPathFailed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

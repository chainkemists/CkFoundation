#pragma once

#include "CkNavigation/Nav/CkNav_Fragment.h"
#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkNav_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Public BP API for CkNavigation. No typesafe handle: pathfinding is a service on any entity
// with a Transform feature; the result fragment is added lazily on first request.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNAVIGATION_API UCk_Utils_Nav_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Nav_UE);

public:
    // Deferred: drained next tick by FProcessor_Nav_HandleRequests, which fires
    // Nav_OnPathReady / Nav_OnPathFailed. Server-authoritative (client -> NotAuthority) and
    // requires CkEcsExt's Transform feature for the start location (absent -> StartProjectFailed).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request FindPath",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_FindPath(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Nav_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Default-constructed result (Status == None) if the entity has never issued a request.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Get Path Result")
    static FCk_Nav_PathResult
    Get_PathResult(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Get Path Status")
    static ECk_Nav_PathStatus
    Get_PathStatus(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Has Path")
    static bool
    Has_Path(
        const FCk_Handle& InHandle);

    // Autotest hook for the deferred-request queue: a rebuild in flight makes the next
    // Request_FindPath land while the start point is unbakeable. Production never needs this.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request Rebuild (Testing)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_NavigationRebuild_ForTesting(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // OutSnappedPosition is written only when a navmesh tile is found within the search box.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Try Project Onto Navmesh")
    static bool
    Try_ProjectOntoNavmesh(
        UPARAM(ref) FCk_Handle& InHandle,
        FVector InWorldPosition,
        float InHalfExtentUu,
        FVector& OutSnappedPosition,
        float InVerticalHalfExtentUu = -1.0f);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Bind To OnPathReady")
    static FCk_Handle
    BindTo_OnPathReady(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Unbind From OnPathReady")
    static FCk_Handle
    UnbindFrom_OnPathReady(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Bind To OnPathFailed")
    static FCk_Handle
    BindTo_OnPathFailed(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Unbind From OnPathFailed")
    static FCk_Handle
    UnbindFrom_OnPathFailed(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

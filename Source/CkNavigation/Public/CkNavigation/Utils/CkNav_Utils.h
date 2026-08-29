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

class AActor;

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

    // The release half of Request_FindPath. Returns the result slot to None, drops the entity's
    // queued and deferred queries, and stamps the caller's post-abandon revision so a query that
    // drains afterwards is recognised as superseded. Immediate, not deferred: the caller is
    // ending the episode now, and leaving the slot readable as Pending for even one more tick is
    // what every consumer of Get_PathStatus would act on.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request AbandonPath",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_AbandonPath(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Nav_AbandonPath& InRequest,
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

    // Autotest hooks for the pending watchdog. Its two rows guard states that, once every terminal
    // releases its episode correctly, CANNOT be produced through the public API — an orphaned
    // Pending slot is unreachable by construction, and no provider stalls past the timeout on
    // demand. A reconciler whose whole job is converging from arbitrary state has to be driven
    // from arbitrary state to be tested at all. Production never needs either of these.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request MarkPathPending (Testing)")
    static FCk_Handle
    Request_MarkPathPending_ForTesting(
        UPARAM(ref) FCk_Handle& InHandle,
        int32 InRequestRevision);

    // Backdates the parked-at timestamp so the timeout row can be driven against the REAL
    // threshold in one frame. Only the clock is faked; the episode, tags, revision and threshold
    // are all production state.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request AgePathPending (Testing)")
    static FCk_Handle
    Request_AgePathPending_ForTesting(
        UPARAM(ref) FCk_Handle& InHandle,
        float InAgeBySeconds);

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

    // Actor-level registration is required for nav-relevant actors such as NavModifierVolumes,
    // whose BrushComponent contribution is not controlled by a NavModifierComponent toggle.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request Set Actor Navigation Registered")
    static void
    Request_SetActorNavigationRegistered(
        AActor* InActor,
        bool InRegistered);

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

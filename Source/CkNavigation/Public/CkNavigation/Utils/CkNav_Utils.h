#pragma once

#include "CkNavigation/Nav/CkNav_Fragment.h"
#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkNav_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Public BP API for CkNavigation. CkNavigation does not own a typesafe handle —
// pathfinding is a service exposed to any entity that has a Transform feature
// (CkEcsExt) and the requisite path-result fragment slot. The Utils helpers add
// the result-fragment lazily on first request.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNAVIGATION_API UCk_Utils_Nav_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Nav_UE);

public:
    // Enqueue a FindPath request on this entity. Adds FFragment_Nav_PathResult +
    // FFragment_Nav_Requests if missing; the request is drained next tick by
    // FProcessor_Nav_HandleRequests. Result lands in FFragment_Nav_PathResult; the
    // Nav_OnPathReady / Nav_OnPathFailed signals fire on completion.
    //
    // Server-authoritative: client-side requests fail with NotAuthority.
    //
    // The entity must have CkEcsExt's Transform feature so the processor can read
    // the start location. Without it the request fails with StartProjectFailed.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request FindPath")
    static FCk_Handle
    Request_FindPath(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_Nav_FindPath& InRequest);

    // Read the current path result. Returns a default-constructed result if the
    // entity has never issued a request (Status == None).
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

public:
    // Bind a delegate to fire whenever a FindPath request on this entity succeeds.
    // The delegate fires with the entity handle + the new path result.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Bind To OnPathReady")
    static FCk_Handle
    BindTo_OnPathReady(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightthisFrame,
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
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightthisFrame,
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

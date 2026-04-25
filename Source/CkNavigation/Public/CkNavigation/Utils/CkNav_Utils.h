#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkNav_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Nav_HandleRequests; }
struct FCk_Handle_Transform;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_NavAgent"))
class CKNAVIGATION_API UCk_Utils_Nav_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Nav_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_NavAgent);

public:
    friend class ck::FProcessor_Nav_HandleRequests;

public:
    // ----------------------------------------------------------------------------------------------------------------
    // Has / Cast / DoCast / DoCastChecked / Get_InvalidHandle
    // ----------------------------------------------------------------------------------------------------------------
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_NavAgent
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Handle -> NavAgent Handle",
              meta = (CompactNodeTitle = "<AsNavAgent>", BlueprintAutocast))
    static FCk_Handle_NavAgent
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid NavAgent Handle",
              Category = "Ck|Utils|Nav",
              meta = (CompactNodeTitle = "INVALID_NavAgentHandle", Keywords = "make"))
    static FCk_Handle_NavAgent
    Get_InvalidHandle() { return {}; };

public:
    // ----------------------------------------------------------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

    // Add the nav-agent feature to a transform entity. Crowd registration happens next frame
    // via FTag_Nav_NeedsSetup. Authority-gated; double-add guarded (P3-7).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Add Feature")
    static FCk_Handle_NavAgent
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Nav_AgentParams& InParams);

    // ----------------------------------------------------------------------------------------------------------------
    // Path requests
    // ----------------------------------------------------------------------------------------------------------------

    // Queue a path-find request. Result lands on FFragment_Nav_PathResult within the same frame
    // (FProcessor_Nav_HandleRequests runs in FGroup_PostTransform; path query is synchronous).
    // Authority-gated.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Request Find Path")
    static FCk_Handle_NavAgent
    Request_FindPath(
        UPARAM(ref) FCk_Handle_NavAgent& InHandle,
        const FCk_Request_Nav_FindPath& InRequest);

    // ----------------------------------------------------------------------------------------------------------------
    // Signal bindings
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Bind To OnPathReady")
    static FCk_Handle_NavAgent
    BindTo_OnPathReady(
        UPARAM(ref) FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Unbind From OnPathReady")
    static FCk_Handle_NavAgent
    UnbindFrom_OnPathReady(
        UPARAM(ref) FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Bind To OnPathFailed")
    static FCk_Handle_NavAgent
    BindTo_OnPathFailed(
        UPARAM(ref) FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Unbind From OnPathFailed")
    static FCk_Handle_NavAgent
    UnbindFrom_OnPathFailed(
        UPARAM(ref) FCk_Handle_NavAgent& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate);

    // ----------------------------------------------------------------------------------------------------------------
    // Result accessors
    // (Intentionally guarded: invalid handle / missing fragment returns a sensible default,
    //  no ensures — "fragment not added yet" is a normal frame-N state, not an error.)
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Get Path Status")
    static ECk_Nav_PathStatus
    Get_PathStatus(const FCk_Handle_NavAgent& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Get Waypoints")
    static TArray<FVector>
    Get_Waypoints(const FCk_Handle_NavAgent& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Nav",
              DisplayName = "[Ck][Nav] Get Has Path")
    static bool
    Get_HasPath(const FCk_Handle_NavAgent& InHandle);

    // No runtime nav-bounds setup helper here. UE's nav baking is fundamentally pre-baked +
    // dynamic-update; baking from zero in a Game world doesn't work. Place an
    // ANavMeshBoundsVolume in the test level via the editor instead.
};

// --------------------------------------------------------------------------------------------------------------------

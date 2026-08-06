#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"
#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkPathNetwork_Utils.generated.h"

class UCk_PathNetwork_Detector_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PathNetwork"))
class CKPATHNETWORK_API UCk_Utils_PathNetwork_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PathNetwork_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_PathNetwork);

public:
    // Add the path-network feature to InOwner. Creates a child entity carrying the authored
    // ribbons + build params; FProcessor_PathNetwork_Setup builds the graph on the next tick.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Add Feature")
    static FCk_Handle_PathNetwork
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_PathNetwork_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // Returns a value-only snapshot of every authored and generated ribbon carried by a
    // path-network entity in the same ECS world as InAnyHandleInWorld. An invalid handle
    // returns an empty array so debug consumers can fail closed during world teardown.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Get All Ribbons In World")
    static TArray<FCk_PathNetwork_Ribbon>
    Get_AllRibbonsInWorld(
        FCk_Handle InAnyHandleInWorld);

    // Replace the ribbon set and rebuild the graph (the runtime-rebuild entrypoint). Every
    // follower corridor planned against this network is invalidated and replans.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Request Rebuild",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PathNetwork
    Request_Rebuild(
        UPARAM(ref) FCk_Handle_PathNetwork& InNetwork,
        const FCk_Request_PathNetwork_Rebuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Run a per-game detector over the given bounds, vectorize the mask into Generated ribbons,
    // and rebuild as (existing Authored ribbons + fresh Generated ribbons). Hand-authored content
    // survives re-detection; previously Generated ribbons are replaced wholesale.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Request Rebuild From Detector",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PathNetwork
    Request_RebuildFromDetector(
        UPARAM(ref) FCk_Handle_PathNetwork& InNetwork,
        const UCk_PathNetwork_Detector_UE* InDetector,
        FBox InWorldBounds,
        const FCk_PathNetwork_VectorizeParams& InVectorizeParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // True once the setup processor has built the graph (and it has at least one edge).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Get Is Built")
    static bool
    Get_IsBuilt(
        const FCk_Handle_PathNetwork& InNetwork);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Get Num Nodes")
    static int32
    Get_NumNodes(
        const FCk_Handle_PathNetwork& InNetwork);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Get Num Edges")
    static int32
    Get_NumEdges(
        const FCk_Handle_PathNetwork& InNetwork);

    // Bumps on every (re)build. Corridors planned against an older epoch are stale.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Get Build Epoch")
    static int32
    Get_BuildEpoch(
        const FCk_Handle_PathNetwork& InNetwork);

    // Returns the map-authored follower profile when this network explicitly opted in.
    // Disabled or malformed recommendations fail closed and leave OutTuning at defaults.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] Try Get Recommended Follower Tuning")
    static bool
    TryGet_RecommendedFollowerTuning(
        const FCk_Handle_PathNetwork& InNetwork,
        FCk_PathNetworkFollower_Tuning& OutTuning);

    // Closest point on any edge centerline within InSearchRadius. Returns false if the network
    // is unbuilt or nothing is in range.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetwork",
              DisplayName="[Ck][PathNetwork] TryGet Closest Point On Network")
    static bool
    TryGet_ClosestPointOnNetwork(
        const FCk_Handle_PathNetwork& InNetwork,
        FVector InLocation,
        float InSearchRadius,
        FVector& OutClosestPoint);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PathNetworkFollower"))
class CKPATHNETWORK_API UCk_Utils_PathNetworkFollower_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PathNetworkFollower_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_PathNetworkFollower);

public:
    // Activate path-network following on an entity (fragments are stamped directly on InHandle —
    // the crowd integration expects follower state on the agent entity itself). Without this
    // feature the entity behaves exactly as before; Remove() deactivates it again.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Add Feature")
    static FCk_Handle_PathNetworkFollower
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams);

    // Adds the feature when absent, or reacquires the existing feature when its
    // cooperative owner token matches. A foreign or unowned feature is never
    // mutated, removed, or retuned.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Try Add Or Adopt By Owner Token",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_PathNetworkFollower
    Try_AddOrAdoptByOwnerToken(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_PathNetworkFollower_ParamsData& InParams,
        ECk_PathNetworkFollower_OwnershipResult& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName = "[Ck][PathNetworkFollower] Get Owner Token")
    static FName
    Get_OwnerToken(
        const FCk_Handle_PathNetworkFollower& InFollower);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Get Is Tuning Valid")
    static bool
    Get_IsTuningValid(
        const FCk_PathNetworkFollower_Tuning& InTuning);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Remove Feature")
    static FCk_Handle
    Remove(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower);

    // Plan a preference-weighted route to the goal. Result lands on the follower's corridor
    // fragment; OnRouteReady / OnRouteFailed fire on completion. Server-only (mirrors CkNavigation).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Request Find Route",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PathNetworkFollower
    Request_FindRoute(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Request_PathNetworkFollower_FindRoute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Request Set Network",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PathNetworkFollower
    Request_SetNetwork(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Handle_PathNetwork& InNetwork,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Atomically updates all routing preference values. If this follower has a
    // Ready/Pending route, queues a replacement for its same goal; callers never
    // mutate the backing fragment directly.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName = "[Ck][PathNetworkFollower] Request Update Tuning And Replan")
    static FCk_Handle_PathNetworkFollower
    Request_UpdateTuningAndReplan(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_PathNetworkFollower_Tuning& InTuning);

    // Queue the same atomic tuning update for every follower in this world whose
    // owner token matches. The token is part of the follower feature itself, so
    // callers do not need a secondary tag/index that can drift from ownership.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName = "[Ck][PathNetworkFollower] Request Update Tuning By Owner Token")
    static int32
    Request_UpdateTuningAndReplanByOwnerToken(
        FCk_Handle InAnyHandleInWorld,
        FName InOwnerToken,
        const FCk_PathNetworkFollower_Tuning& InTuning);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Get Route Result")
    static FCk_PathNetwork_RouteResult
    Get_RouteResult(
        const FCk_Handle_PathNetworkFollower& InFollower);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Get Route Status")
    static ECk_PathNetwork_RouteStatus
    Get_RouteStatus(
        const FCk_Handle_PathNetworkFollower& InFollower);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Bind To OnRouteReady")
    static FCk_Handle_PathNetworkFollower
    BindTo_OnRouteReady(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Unbind From OnRouteReady")
    static FCk_Handle_PathNetworkFollower
    UnbindFrom_OnRouteReady(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteReady& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Bind To OnRouteFailed")
    static FCk_Handle_PathNetworkFollower
    BindTo_OnRouteFailed(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PathNetworkFollower",
              DisplayName="[Ck][PathNetworkFollower] Unbind From OnRouteFailed")
    static FCk_Handle_PathNetworkFollower
    UnbindFrom_OnRouteFailed(
        UPARAM(ref) FCk_Handle_PathNetworkFollower& InFollower,
        const FCk_Delegate_PathNetworkFollower_OnRouteFailed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

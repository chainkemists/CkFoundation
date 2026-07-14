#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include "CkEntityReplicationDriver_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EntityReplicationDriver_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityReplicationDriver_UE);

public:
    static auto
    TryAdd(
        FCk_Handle& InHandle) -> ECk_AddedOrNot;

public:
    static auto
    Get_NumOfReplicationDriversIncludingDependents(
        const FCk_Handle& InHandle) -> int32;

    // Fire-gating (spec §4.4, CTO note N2): true while THIS entity OR any lifetime-dependent still has replicated
    // fragments pending apply (FTag_RepFragments_PendingApply — which also covers queued removals) or a queued
    // save-load hydration payload (FFragment_PendingHydration). Recurses the lifetime-dependents tree — the same
    // traversal Get_NumOfReplicationDriversIncludingDependents derives the expected-dependent count from, so at
    // gate-check time (fire tag set once all dependents are synced) every dependent is reachable. Cross-registry
    // children are excluded from the lifetime-dependents set, so nested-registry entities are invisible here (they
    // carry no driver on this wire — acceptable). Framework-internal processor plumbing, not public API.
    static auto
    Get_HasUndrainedReplicatedFragments_IncludingDependents(
        const FCk_Handle& InHandle) -> bool;

public:
    UFUNCTION(BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "Ck|Utils|ReplicationDriver",
        DisplayName="[Ck][ReplicationDriver] Request Build and Replicate")
    static FCk_Handle
    Request_BuildAndReplicate(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo);

    UFUNCTION(BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "Ck|Utils|ReplicationDriver",
        DisplayName="[Ck][ReplicationDriver] Request Build and Replicate (Composite)")
    static FCk_Handle
    Request_BuildAndReplicate_Multiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos);

public:
    static auto
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild = nullptr) -> FCk_Handle;

    static auto
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild = nullptr) -> FCk_Handle;

    // Restore-path sibling of Request_TryBuildAndReplicate for an EXISTING entity (snapshot-restored):
    // the entity's fragments were already reconstituted from snapshot bytes, so this SKIPS entity
    // creation and construction-script execution and only re-establishes the replication half —
    // driver fragment + the ReplicationData that lets clients construct their mirror from the
    // ConstructionInfos. Idempotent w.r.t. the driver (TryAdd); call once per restored entity, after
    // the OWNER's driver is re-established (snapshot respawn pass).
    static auto
    Request_TryReplicateExisting(
        FCk_Handle& InExistingEntity,
        FCk_Handle& InReplicatedOwner,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos) -> void;

    static auto
    Request_Replicate(
        FCk_Handle& InHandleToReplicate,
        FCk_Handle InReplicatedOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScript,
        const FInstancedStruct& InSpawnParams,
        const TOptional<FCk_Handle>& InReplicatedContextOwnerOverride = {}) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Has Feature")
    static bool
    Has(
        const FCk_Handle& InEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Ensure Has Feature")
    static bool
    Ensure(
        const FCk_Handle& InEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Get_IsReplicationComplete")
    static bool
    Get_IsReplicationComplete(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Get_IsReplicationComplete (On All Dependents)")
    static bool
    Get_IsReplicationCompleteAllDependents(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Promise/Future OnReplicationComplete")
    static void
    Promise_OnReplicationComplete(
        UPARAM(ref) FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="[Ck][ReplicationDriver] Promise/Future OnReplicationComplete (On All Dependents)")
    static void
    Promise_OnReplicationCompleteAllDependents(
        UPARAM(ref) FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate);

public:
    static auto
    Get_ReplicatedHandleForWorld(
        const FCk_Handle& InHandle,
        const UWorld* InHandleOwningWorld,
        const UWorld* InTargetWorld) -> FCk_Handle;
};

// --------------------------------------------------------------------------------------------------------------------

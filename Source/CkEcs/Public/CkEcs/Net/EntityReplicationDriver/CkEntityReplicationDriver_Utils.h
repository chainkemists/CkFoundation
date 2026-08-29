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

    // Framework-internal fire-gating, not public API: true while this entity or any lifetime-dependent still
    // has replicated fragments pending apply or a queued hydration payload. Cross-registry children are not
    // in the lifetime-dependents set, so nested-registry entities are invisible here (they carry no driver).
    static auto
    Get_HasUndrainedReplicatedFragments_IncludingDependents(
        const FCk_Handle& InHandle) -> bool;

    // Diagnostic twin of the above: the FIRST still-undrained entity in the tree, invalid handle if none
    static auto
    TryGet_FirstUndrainedReplicatedFragmentsEntity_IncludingDependents(
        const FCk_Handle& InHandle) -> FCk_Handle;

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

    // Restore-path sibling of Request_TryBuildAndReplicate for an ALREADY-reconstituted entity: skips entity
    // creation and construction scripts, re-establishing only the driver + ReplicationData clients mirror
    // from. Idempotent; call once per restored entity, AFTER the owner's driver is re-established.
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

namespace ck::entity_replication_driver
{
    // Runs ONE construction step onto InEntity, and is the only place that decides what to do when the step's
    // archetype is absent. Both construction sites - the authority's Request_BuildAndReplicate and the client's
    // OnRep - go through here, because an archetype that resolves on one and not the other is precisely the
    // divergence that produces a half-built entity nobody can explain.
    //
    // Falling back to the class default is the LAST resort and is deliberately silent here: the caller that knows
    // the step came from a save (the loader) has already diagnosed the miss and marked the entity for reaping, and
    // a step that legitimately carries no archetype at all is the ordinary case.
    CKECS_API auto
    Construct_FromInfo(
        const FCk_EntityReplicationDriver_ConstructionInfo& InInfo,
        FCk_Handle& InEntity) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

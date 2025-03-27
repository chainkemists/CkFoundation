#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include "CkEntityReplicationDriver_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_EntityReplicationDriver_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityReplicationDriver_UE);

public:
    static auto
    Add(
        FCk_Handle& InHandle) -> ECk_AddedOrNot;

public:
    static auto
    Get_NumOfReplicationDriversIncludingDependents(
        const FCk_Handle& InHandle) -> int32;

public:
    UFUNCTION(BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "Ck|Utils|ReplicationDriver",
        DisplayName="[Ck][ReplicationDriver] Request Build and Replicate")
    static FCk_Handle
    Request_BuildAndReplicate(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo);

public:
    static auto
    Request_TryReplicateAbility(
        FCk_Handle& InHandle,
        const UCk_Entity_ConstructionScript_PDA* InConstructionScript,
        // ideally, this would be UCk_Ability_Script_PDA, however we cannot depend on it in this module
        const TSubclassOf<UCk_DataAsset_PDA>& InAbilityScriptClass,
        const FCk_Handle& InAbilitySource,
        ECk_ConstructionPhase InAbilityConstructionPhase) -> FCk_Handle;

    static auto
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild = nullptr) -> FCk_Handle;

    static auto
    Request_Replicate(
        FCk_Handle& InHandleToReplicate,
        FCk_Handle InReplicatedOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScript) -> void;

    static auto
    Request_ReplicateEntityOnReplicatedActor(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InConstructionInfo) -> void;

    static auto
    Request_ReplicateEntityOnNonReplicatedActor(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor& InConstructionInfo) -> void;

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
              DisplayName="[Ck][ReplicationDriver] Get_IsReplicationCompleteOnAllDependents")
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
              DisplayName="[Ck][ReplicationDriver] Promise/Future OnReplicationCompleteAllDependents")
    static void
    Promise_OnReplicationCompleteAllDependents(
        UPARAM(ref) FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

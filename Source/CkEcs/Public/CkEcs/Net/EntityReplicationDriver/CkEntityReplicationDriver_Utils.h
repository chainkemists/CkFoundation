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

    static auto
    Request_Replicate(
        FCk_Handle& InHandleToReplicate,
        FCk_Handle InReplicatedOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScript,
        const FInstancedStruct& InSpawnParams) -> void;

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

#pragma once

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
        FCk_Handle InHandle) -> void;

public:
    static auto
    Get_NumOfReplicationDriversIncludingDependents(
        FCk_Handle InHandle) -> int32;

public:
    static auto
    Request_Replicate(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo) -> void;

    static auto
    Request_ReplicateEntityOnReplicatedActor(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InConstructionInfo) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="Has ReplicationDriver")
    static bool
    Has(
        FCk_Handle InEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ReplicationDriver",
              DisplayName="Ensure Has ReplicationDriver")
    static bool
    Ensure(
        FCk_Handle InEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ReplicationDriver")
    static void
    Promise_OnReplicationComplete(
        FCk_Handle InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ReplicationDriver")
    static void
    Promise_OnReplicationCompleteAllDependents(
        FCk_Handle InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

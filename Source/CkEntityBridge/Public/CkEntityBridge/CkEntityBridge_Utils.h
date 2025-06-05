#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include "CkEntityBridge_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKENTITYBRIDGE_API UCk_Utils_EntityBridge_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityBridge_UE);

public:
    static void
    Request_Spawn(
        const FCk_Handle& InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_EntityBridge_OnEntitySpawned& InDelegate);

public:
    // this SHOULD be in NetUtils but because it depends on EntityBridge, the function is in EntityBridge utils file
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Does Actor's Entity Replicate",
              Category = "Ck|Utils|Net")
    static ECk_Utils_Net_ActorEntityReplicationStatus
    Get_DoesActorEntityReplicate(
        AActor* InActor);
};

// --------------------------------------------------------------------------------------------------------------------

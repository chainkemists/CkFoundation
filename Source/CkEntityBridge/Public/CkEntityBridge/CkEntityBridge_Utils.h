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
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityBridge",
              DisplayName  = "[Ck] Request Spawn New Entity",
              meta = (AutoCreateRefTerm = "InOptionalPayload, InDelegate"))
    static void
    Request_Spawn(
        const FCk_Handle& InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_EntityBridge_OnEntitySpawned& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

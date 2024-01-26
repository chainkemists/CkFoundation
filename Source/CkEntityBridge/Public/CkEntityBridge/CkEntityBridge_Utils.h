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
              DisplayName  = "[Ck] Request Spawn New Entity")
    static void
    Request_Spawn(
        FCk_Handle InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest);

    UFUNCTION(BlueprintCallable,
              DisplayName  = "[Ck] Build Entity",
              Category = "Ck|Utils|EntityBridge")
    static FCk_Handle
    BuildEntity(
        FCk_Handle InHandle,
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------

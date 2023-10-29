#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Params.h"

#include "CkEntityBridge_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUNREAL_API UCk_Utils_EntityBridge_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityBridge_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityBridge",
        DisplayName  = "Request Spawn Entity")
    static void
    Request_Spawn(
        FCk_Handle InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------

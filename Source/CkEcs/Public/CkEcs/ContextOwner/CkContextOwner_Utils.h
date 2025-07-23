#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/ContextOwner/CkContextOwner_Fragment.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkContextOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_ContextOwner_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ContextOwner_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Context] Get Entity Context Owner",
              Category = "Ck|Utils|Context",
              meta = (CompactNodeTitle = "CTX"))
    static FCk_Handle
    Get_ContextOwner(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Context] Request Override Entity Context Owner",
              Category = "Ck|Utils|Context",
              meta = (Keywords = "set,change,update"))
    static void
    Request_Override(
        UPARAM(ref) FCk_Handle& InEntity,
        const FCk_Handle& InNewContextOwner);

public:
    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

    static auto
    Request_SetupEntityWithContextOwner(
        FCk_Handle& InNewEntity,
        const FCk_Handle& InContextOwner) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
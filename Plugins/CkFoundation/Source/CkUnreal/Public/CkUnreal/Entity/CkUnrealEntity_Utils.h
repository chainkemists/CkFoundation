#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

#include "CkUnrealEntity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUNREAL_API UCk_Utils_UnrealEntity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UnrealEntity_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|UnrealEntity",
        DisplayName  = "Request Spawn Entity")
    static void
    Request_Spawn(FCk_Handle InHandle, const FCk_Request_UnrealEntity_Spawn& InRequest);
};
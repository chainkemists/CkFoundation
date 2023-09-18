#pragma once

#include "GameplayTagContainer.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIntent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKINTENT_API UCk_Utils_Intent_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Intent_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Intent",
              DisplayName = "Add Intent Fragment")
    static void
    Request_Add(
        FCk_Handle   InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Intent",
              DisplayName = "Add New Intent")
    static void
    Request_AddNewIntent(
        FCk_Handle   InHandle,
        FGameplayTag InIntent);
};

// --------------------------------------------------------------------------------------------------------------------

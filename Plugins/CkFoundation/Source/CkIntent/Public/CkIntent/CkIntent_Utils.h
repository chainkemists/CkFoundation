#pragma once

#include "GameplayTagContainer.h"

#include "CkMacros/CkMacros.h"

#include "CkIntent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKINTENT_API UCk_Utils_Intent_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Intent_UE);

public:
    static auto
    Request_Add(
        FCk_Handle   InHandle) -> void;

    static auto
    Request_AddNewIntent(
        FCk_Handle   InHandle,
        FGameplayTag InIntent) -> void;
};

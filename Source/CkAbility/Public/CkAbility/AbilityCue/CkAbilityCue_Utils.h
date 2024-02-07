#pragma once

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkAbilityCue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityCue_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AbilityCue_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AbilityCue",
        DisplayName="[Ck][AbilityCue] Request Spawn",
        meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static void
    Request_Spawn_AbilityCue(
        const FCk_Handle& InHandle,
        FGameplayTag InAbilityCueName,
        FCk_AbilityCue_Params InReplicatedParams,
        UObject* InWorldContextObject);

    static void
    Request_Spawn_AbilityCue(
        const FCk_Handle& InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AbilityCue",
        DisplayName="[Ck][AbilityCue] Get Params")
    static FCk_AbilityCue_Params
    Get_Params(
        const FCk_Handle& InAbilityCueEntity);
};

// --------------------------------------------------------------------------------------------------------------------

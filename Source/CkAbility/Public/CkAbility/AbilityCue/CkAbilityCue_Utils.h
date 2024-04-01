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
        BlueprintAuthorityOnly,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][AbilityCue] Request Spawn")
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

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Make AbilityCue Params",
        Category = "Ck|Utils|AbilityCue",
        meta = (NativeMakeFunc))
    static FCk_AbilityCue_Params
    Make_AbilityCue_Params(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser);
};

// --------------------------------------------------------------------------------------------------------------------

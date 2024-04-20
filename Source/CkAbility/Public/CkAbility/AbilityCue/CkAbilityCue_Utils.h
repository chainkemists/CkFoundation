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

    UFUNCTION(BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][AbilityCue] Request Spawn (Local)")
    static void
    Request_Spawn_AbilityCue_Local(
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

    UFUNCTION(BlueprintPure,
        CustomThunk,
        DisplayName = "[Ck] Make AbilityCue Params With Custom Data",
        Category = "Ck|Utils|AbilityCue",
        meta=(CustomStructureParam = "InValue", BlueprintInternalUseOnly = true))
    static FCk_AbilityCue_Params
    INTERNAL__Make_AbilityCue_Params_With_CustomData(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser,
        const int32& InValue);
    DECLARE_FUNCTION(execINTERNAL__Make_AbilityCue_Params_With_CustomData);

    static auto
    Make_AbilityCue_Params_With_CustomData(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser,
        FInstancedStruct InCustomData) -> FCk_AbilityCue_Params;

    UFUNCTION(BlueprintPure,
        CustomThunk,
        DisplayName = "[Ck] Make AbilityCue With Custom Data",
        Category = "Ck|Utils|AbilityCue",
        meta=(CustomStructureParam = "InValue", BlueprintInternalUseOnly = true))
    static FCk_AbilityCue_Params
    INTERNAL__Make_AbilityCue_With_CustomData(
        const int32& InValue);
    DECLARE_FUNCTION(execINTERNAL__Make_AbilityCue_With_CustomData);

    static auto
    Make_AbilityCue_With_CustomData(
        FInstancedStruct InCustomData) -> FCk_AbilityCue_Params;

};

// --------------------------------------------------------------------------------------------------------------------

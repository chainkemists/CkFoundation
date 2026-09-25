#pragma once

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProceduralRig_Utils.generated.h"

UCLASS(NotBlueprintable, meta = (ScriptMixin = "FCk_Handle_ProceduralRig"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralRig_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralRig_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralRig);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|ProceduralRig")
    static FCk_Handle_ProceduralRig
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_ProceduralRig_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralRig")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralRig")
    static bool
    Get_IsReady(
        const FCk_Handle_ProceduralRig& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralRig")
    static ECk_ProceduralRig_Failure
    Get_Failure(
        const FCk_Handle_ProceduralRig& InHandle);

private:

    UFUNCTION(BlueprintCallable, Category = "Ck|ProceduralRig", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralRig
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralRig", meta = (BlueprintAutocast, CompactNodeTitle = "<AsProceduralRig>"))
    static FCk_Handle_ProceduralRig
    DoCastChecked(
        FCk_Handle InHandle);
};

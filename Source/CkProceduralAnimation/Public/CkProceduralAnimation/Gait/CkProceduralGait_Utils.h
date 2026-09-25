#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProceduralGait_Utils.generated.h"

UCLASS(NotBlueprintable, meta = (ScriptMixin = "FCk_Handle_ProceduralGait"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralGait_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralGait_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralGait);

public:
    // Adds onto an existing transform entity. The caller owns movement and all rendered parts.

    UFUNCTION(BlueprintCallable, Category = "Ck|ProceduralGait")
    static FCk_Handle_ProceduralGait
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_ProceduralGait_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static bool
    Get_IsReady(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static bool
    Get_HasFailed(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static int32
    Get_TrustedContactCount(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static int32
    Get_PlantedCount(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static TArray<FCk_ProceduralGait_Foot>
    Get_Feet(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static float
    Get_GaitClock(
        const FCk_Handle_ProceduralGait& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait")
    static TArray<FCk_ProceduralGait_Leg>
    Get_Legs(
        const FCk_Handle_ProceduralGait& InHandle);

private:

    UFUNCTION(BlueprintCallable, Category = "Ck|ProceduralGait", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralGait
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|ProceduralGait", meta = (BlueprintAutocast, CompactNodeTitle = "<AsProceduralGait>"))
    static FCk_Handle_ProceduralGait
    DoCastChecked(
        FCk_Handle InHandle);
};

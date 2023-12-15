#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkArithmetic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Arithmetic_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Arithmetic_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_LerpFloatWithEasing(
        float InA,
        float InB,
        float InAlpha,
        float InPower,
        ECk_EasingMethod InEasingMethod);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_RoundFloatToFloat(
        ECk_RoundingMethod InRoundingMethod,
        float InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_RoundFloatToInt(
        ECk_RoundingMethod InRoundingMethod,
        float InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_WrapAroundModulo(
        int32 InA,
        int32 InB);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T>
    auto
    Negate(const T& InValue) -> T;
}

// --------------------------------------------------------------------------------------------------------------------

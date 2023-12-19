#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

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
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Math|Arithmetic",
        meta=(CompactNodeTitle="++Wrap"))
    static void
    Increment_WithWrap(
        UPARAM(Ref) int32& InToIncrement,
        const FCk_IntRange& InRange);

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
    Get_Increment_WithWrap(
        int32 InToIncrement,
        const FCk_IntRange& InRange);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T>
    auto
    Negate(const T& InValue) -> T;
}

// --------------------------------------------------------------------------------------------------------------------

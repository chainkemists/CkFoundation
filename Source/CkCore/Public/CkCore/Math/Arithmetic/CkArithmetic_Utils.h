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
        DisplayName = "[Ck] Increment With Wrap",
        meta=(CompactNodeTitle="++Wrap"))
    static int32
    Increment_WithWrap(
        UPARAM(Ref) int32& InToIncrement,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Math|Arithmetic",
        DisplayName = "[Ck] Decrement With Wrap",
        meta=(CompactNodeTitle="--Wrap"))
    static int32
    Decrement_WithWrap(
        UPARAM(Ref) int32& InToDecrement,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Math|Arithmetic",
        DisplayName = "[Ck] Offset With Wrap",
        meta=(CompactNodeTitle="++Wrap"))
    static int32
    Offset_WithWrap(
        UPARAM(Ref) int32& InToJump,
        int32 InAmountToOffset,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Lerp Float With Easing",
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_LerpFloatWithEasing(
        float InA,
        float InB,
        float InAlpha,
        float InPower,
        ECk_EasingMethod InEasingMethod);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle From Dot Product (Degrees)",
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_AngleFromDotProduct_Degrees(
        float InDotProduct);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle From Dot Product (Radians)",
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_AngleFromDotProduct_Radians(
        float InDotProduct);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Round Float To Float",
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_RoundFloatToFloat(
        ECk_RoundingMethod InRoundingMethod,
        float InValue);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Round Float To Int",
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_RoundFloatToInt(
        ECk_RoundingMethod InRoundingMethod,
        float InValue);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Increment With Wrap",
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_Increment_WithWrap(
        int32 InToIncrement,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Decrement With Wrap",
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_Decrement_WithWrap(
        int32 InToDecrement,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Offset With Wrap",
              Category = "Ck|Utils|Math|Arithmetic")
    static int32
    Get_Offset_WithWrap(
        int32 InToJump,
        int32 InOffset,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Default Value if Zero",
              Category = "Ck|Utils|Math|Arithmetic")
    static float
    Get_DefaultIfZero(
        float InValue,
        float InDefaultIfZero,
        float InTolerance = 0.0001f);

public:
    static auto
    Get_IsNearlyEqual(uint8 A, uint8 B) -> bool;

    static auto
    Get_IsNearlyEqual(int32 A, int32 B) -> bool;

    static auto
    Get_IsNearlyEqual(float A, float B) -> bool;

    static auto
    Get_IsNearlyEqual(double A, double B) -> bool;

    static auto
    Get_IsNearlyEqual(const FVector& A, const FVector& B) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T>
    auto
    Negate(const T& InValue) -> T;
}

// --------------------------------------------------------------------------------------------------------------------

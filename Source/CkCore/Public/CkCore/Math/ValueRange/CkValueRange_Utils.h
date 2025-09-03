#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkValueRange_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_IntRange"))
class CKCORE_API UCk_Utils_IntRange_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntRange_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Within Range (Int)",
              Category = "Ck|Utils|Math|IntRange")
    static bool
    Get_IsWithinRange(
        int32 InValue,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Value In Range (Int)",
              Category = "Ck|Utils|Math|IntRange")
    static int32
    Get_RandomValueInRange(
        const FCk_IntRange& InRange);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|IntRange",
              DisplayName = "[Ck] IntRange -> Vector2",
              meta = ( BlueprintAutocast,  CompactNodeTitle = "->"))
    static FVector2D
    Conv_IntRangeToVector2D(
        const FCk_IntRange& InRange);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_FloatRange"))
class CKCORE_API UCk_Utils_FloatRange_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatRange_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Within Range (Float)",
              Category = "Ck|Utils|Math|FloatRange")
    static bool
    Get_IsWithinRange(
        float InValue,
        const FCk_FloatRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Value In Range (Float)",
              Category = "Ck|Utils|Math|FloatRange")
    static float
    Get_RandomValueInRange(
        const FCk_FloatRange& InRange);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRange",
              DisplayName = "[Ck] FloatRange -> Vector2",
              meta = (BlueprintAutocast,  CompactNodeTitle = "->"))
    static FVector2D
    Conv_FloatRangeToVector2D(
        const FCk_FloatRange& InRange);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatRange_0to1_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatRange_0to1_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRange0to1",
              meta = (NativeMakeFunc))
    static FCk_FloatRange_0to1
    // ReSharper disable once CppInconsistentNaming
    Make_FloatRange_0to1(
        float In0to1);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatRange_Minus1to1_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatRange_Minus1to1_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRangeMinus1to1",
              meta = (NativeMakeFunc))
    static FCk_FloatRange_Minus1to1
    // ReSharper disable once CppInconsistentNaming
    Make_FloatRange_Minus1to1(
        float InMinus1to1);
};

// --------------------------------------------------------------------------------------------------------------------

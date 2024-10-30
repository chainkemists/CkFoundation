#pragma once

#include "CkCore/Math/Comparison/CkComparison.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkComparison_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_IntComparison_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntComparison_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Comparison True (Int)",
              Category = "Ck|Utils|Math|IntComparison")
    static bool
    Get_IsComparisonTrue(
        int32 InLHS,
        const FCk_Comparison_Int& InComparison);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is In Range (Int)",
              Category = "Ck|Utils|Math|IntComparison")
    static bool
    Get_IsInRange(
        int32 InNum,
        const FCk_Comparison_IntRange& InComparison);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatComparison_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatComparison_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Comparison True (Float)",
              Category = "Ck|Utils|Math|FloatComparison")
    static bool
    Get_IsComparisonTrue(
        float InLHS,
        const FCk_Comparison_Float& InComparison);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is In Range (Float)",
              Category = "Ck|Utils|Math|FloatComparison")
    static bool
    Get_IsInRange(
        float InNum,
        const FCk_Comparison_FloatRange& InComparison);

	UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Less Than Or Nearly Equal (Float)",
              Category = "Ck|Utils|Math|FloatComparison",
              meta = (Keywords = "<= less nearly equal", AdvancedDisplay=ErrorTolerance))
    static bool
    Get_IsLessThanOrNearlyEqual(
        float A,
        float B,
        float ErrorTolerance = 1.e-6f);

	UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Greater Than Or Nearly Equal (Float)",
              Category = "Ck|Utils|Math|FloatComparison",
              meta = (Keywords = ">= less nearly equal", AdvancedDisplay=ErrorTolerance))
    static bool
    Get_IsGreaterThanOrNearlyEqual(
        float A,
        float B,
        float ErrorTolerance = 1.e-6f);
};

// --------------------------------------------------------------------------------------------------------------------
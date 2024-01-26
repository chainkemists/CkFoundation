#pragma once

#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <Curves/CurveFloat.h>

#include "CkFloatCurve_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatCurve_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatCurve_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Value At Time (FloatCurve)",
              Category = "Ck|Utils|Math|FloatCurve")
    static float
    Get_ValueAtTime(
        const UCurveFloat* InFloatCurve,
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Time Range (FloatCurve)",
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_TimeRange(
        const UCurveFloat* InFloatCurve);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Value Range (FloatCurve)",
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_ValueRange(
        const UCurveFloat* InFloatCurve);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_RuntimeFloatCurve_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RuntimeFloatCurve_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Value At Time (RuntimeFloatCurve)",
              Category = "Ck|Utils|Math|RuntimeFloatCurve")
    static float
    Get_ValueAtTime(
        const FRuntimeFloatCurve& InFloatCurve,
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Time Range (RuntimeFloatCurve)",
              Category = "Ck|Utils|Math|RuntimeFloatCurve")
    static FCk_FloatRange
    Get_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Value Range (RuntimeFloatCurve)",
              Category = "Ck|Utils|Math|RuntimeFloatCurve")
    static FCk_FloatRange
    Get_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Valid (RuntimeFloatCurve)",
              Category = "Ck|Utils|Math|RuntimeFloatCurve",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid(
        const FRuntimeFloatCurve& InFloatCurve);
};

// --------------------------------------------------------------------------------------------------------------------

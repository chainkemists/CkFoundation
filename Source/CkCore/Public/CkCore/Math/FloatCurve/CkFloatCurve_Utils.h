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
              DisplayName = "[Ck] Get Runtime Float Curve Value At Time",
              Category = "Ck|Utils|Math|FloatCurve")
    static float
    Get_RuntimeFloatCurve_ValueAtTime(
        const FRuntimeFloatCurve& InFloatCurve,
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Runtime Float Curve Time Range",
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_RuntimeFloatCurve_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Runtime Float Curve Value Range",
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_RuntimeFloatCurve_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Runtime Float Is Valid",
              Category = "Ck|Utils|Math|FloatCurve")
    static bool
    Get_IsValid_RuntimeFloatCurve(
        const FRuntimeFloatCurve& InFloatCurve);
};

// --------------------------------------------------------------------------------------------------------------------

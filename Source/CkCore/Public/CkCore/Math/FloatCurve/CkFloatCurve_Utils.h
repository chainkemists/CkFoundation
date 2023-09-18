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
              Category = "Ck|Utils|Math|FloatCurve")
    static float
    Get_RuntimeFloatCurve_ValueAtTime(
        const FRuntimeFloatCurve& InFloatCurve,
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_RuntimeFloatCurve_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatCurve")
    static FCk_FloatRange
    Get_RuntimeFloatCurve_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve);
};

// --------------------------------------------------------------------------------------------------------------------
